function [ signal, varargout ] = load_bcimat( filename, varargin )
%LOAD_BCIMAT Load a matlab file written by bci_stream2mat.
%
%  [ signal, state1, state2, ... ] =
%     load_bcimat( 'filename', output_dimensions, 'state1', 'state2', ... )
%
%  loads signal and state data from the file given in the first argument.
%
%  In the second argument, specify the number of dimensions that you want
%  the signal output variable to have; use 2 for EEG-like data (samples
%  by channels) and 3 for spectral data (blocks by bins by channels).
%  The default is 3.
%
%  Remaining arguments are treated as BCI2000 state names; the associated 
%  state data will be written into the variables specified as remaining 
%  output arguments.
%  State variables will always be one-dimensional, with their number of 
%  entries matching the first dimension of the signal variable.
%  
%  This file is a supplement to the bci_stream2mat command line tool which
%  is part of the BCI2000 project (http://www.bciresearch.org).
%
%  Author: juergen.mellinger@uni-tuebingen.de
%  Date:   May 16, 2005

if( nargin < 1 )
  error( 'No file name given.' );
  return;
else
  if( nargin < 2 )
    outdim = 3;
    num_states = 0;
  else
    outdim = varargin{ 1 };
    num_states = nargin - 2;
  end
end
if( num_states ~= nargout - 1 )
  error( 'There must be an output argument for the signal, and one for each state argument.' );
  return;
end

if( char( who( '-file', filename ) ) ~= [ 'Data '; 'Index' ] )
  error( 'Input file is not a bci_stream2mat file.' );
  return;
end
load( filename );

switch outdim
  case 2
    signal = zeros( size( Index.Signal, 2 ) * size( Data, 2 ), size( Index.Signal, 1 ) );
    for( i = 1 : size( Index.Signal, 1 ) )
      signal( :, i ) = reshape( Data( Index.Signal( i, : ), : ), [], 1 );
    end
    for( i = 1 : num_states )
      state = [];
      idx = eval( [ 'Index.' varargin{ i + 1 } ] ); 
      for( j = 1 : size( Index.Signal, 2 ) );
        state = [ state; Data( idx, : ) ];
      end
      varargout{ i } = reshape( state, [], 1 );
    end
    
  case 3
    signal = reshape( Data( Index.Signal.', : ).', ...
      [], size( Index.Signal, 2 ), size( Index.Signal, 1 ) );
    for( i = 1 : num_states )
      idx = eval( [ 'Index.' varargin{ i + 1 } ] ); 
      varargout{ i } = squeeze( Data( idx, : ).' );
    end
    
  otherwise
    error( 'Unsupported number of output dimensions.' );
end
