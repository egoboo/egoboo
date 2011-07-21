rem declare a environment variable ASTYLE which points to the 
rem Artistic Style executible ("C:\Program Files\astyle\bin\AStyle.exe" 
rem or whatever is appropriate)

cd src
%ASTYLE% --mode=c -s4bCSKNwm4pDUoOcZ *.c
%ASTYLE% --mode=c -s4bCSKNwm4pDUoOcZ *.cpp
%ASTYLE% --mode=c -s4bCSKNwm4pDUoOcZ *.inl
%ASTYLE% --mode=c -s4bCSKNwm4pDUoOcZ *.h
cd ..
