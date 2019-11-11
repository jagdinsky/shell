## What is Shell
Shell is a command processor that typically runs in a text window where the user types commands that cause actions.
## Shell principles
Shell can read and execute commands from standart input or from a file with extention ".sh" (a shell script file).
## Shell features (with examples)
### The shells supports:
#### 1. changing directories
cd ..
cd
cd ~
cd path
#### 2. piping
ls -l | sort
#### 3. reading from and modifying a file
ls -l > file.txt
cat file.txt
#### 4. conveyors
ls -l > f1.txt && cp f1.txt f2.txt && cat f2.txt
#### 5. background mode
firefox &
nano&
## Stopping
Press ctrl + c if you need to kill a current process.
Type "quit" or "exit" if you need to close the shell program.
