@set WD=%CD%
@set PYWD=%WD%
@set PARMS=%WD%

@set PROG=%WD%\..\..\..\..\..\..\..\prog
@if exist %PROG% cd %PROG%
@if exist %PROG% goto gotprog

@set PROG=%PYTHONHOME%\..\..\BCI2000\prog
@if exist %PROG% cd %PROG%
@if exist %PROG% goto gotprog
:gotprog
@set PROG=%CD%




cd ..\prog
call portable.bat

@set OnConnect=-
@set OnConnect=%OnConnect% ; LOAD PARAMETERFILE %PARMS%\blah.prm
@set OnConnect=%OnConnect% ; SETCONFIG

start Operator               --OnConnect "%OnConnect%"
start SignalGenerator        --EvaluateTiming=0
::start PythonSource           --PythonSrcWD=%PYWD% --PythonSrcClassFile= --EvaluateTiming=0
start PythonSignalProcessing --PythonSigWD=%PYWD% --PythonSigClassFile=
start PythonApplication      --PythonAppWD=%PYWD% --PythonAppClassFile=posner.py
