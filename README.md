## Implementing a Shell in C

## Author's Information
This repository is affilated with COP4610 at Florida State University and has been constructed by the following students:
* Carlos Pantoja-Malaga
* Stanley Vossler
* Matthew Echenique

## Division of Labor
|Part|Programmers|
|---|---|
|Parsing : Part 1|Carlos, Stanley|
|Environment Variables : Part 2 |Carlos, Matthew|
|Prompt : Part 3|Carlos, Matthew, Stanley|
|Tilde Expansion : Part 4|Stanley, Matthew|
|$PATH Search : Part 5|Stanley, Matthew|
|External Command Execution : Part 6|Stanley, Matthew|
|Input/Output Redirection : Part 7|Carlos, Stanley|
|Piping : Part 8| Carlos, Stanley|
|Background Processing : Part 9| Carlos, Stanley, Matthew|
|Built-in Functions : Part 10| Carlos, Stanley Matthew|

## Project Contributions
|Part|Programmers|
|---|---|
|Parsing : Part 1|N/A|
|Environment Variables : Part 2 |Carlos|
|Prompt : Part 3|N/A|
|Tilde Expansion : Part 4|Carlos|
|$PATH Search : Part 5|Carlos, Stanley|
|External Command Execution : Part 6|Stanley|
|Input/Output Redirection : Part 7|Stanley, Matthew|
|Piping : Part 8|Stanley, Matthew|
|Background Processing : Part 9|Carlos|
|Built-in Functions : Part 10|Stanley, Carlos, Matthew|

## Repository Files
### Folders
Various folders within the repository correspond to iterations designed by the group or specific team members. The folder `group-iterations` corresponds to iterations of the shell with merged code. The folder `carlos-iterations` refers to iterations worked on by Carlos. The folder `matthew-files` corresponds to pseudo code constructed by Matthew. Finaly the folder `images` corresponds to images which appear in this README.md.

### shell.c
This is the C file which contains the program which runs our shell. It is written completely in C and was made to simulate a bash shell and bash shell functionalities.

### makefile
This is the makefile which will result in an executable `./shell`, which when executed will run the program which simulates a bash shell.

### README.md
This is the file you are currently examining. It provides documentation concerning `shell.c`.

## Makefile Compilation 
In order to compile `shell.c` you should download both `shell.c` and `makefile`. In the respective directory you can run the `make` command and then execute the shell through either `shell` or `./shell`.

## Known Bugs & Issues
### Background Processing
The standard output of a background process might affect on how the prompt is presented to the user upon completion of the background process. The expectation is that the output of the process will be displayed followed by the prompt, but in actuality the prompt is displayed before the output of the background process. This is also an issue in BASH, as when the scenario is replicated it produces the same outcome.

The formatting issue as shown in our shell:
![shell formatting issue cause background processing](/images/shell_prompt_bug.png)

The formatting issue replicated in BASH:
![bash formatting issue cause background processing](/images/bash_prompt_bug.png)

### Piping 
Our implementation was modeled after canvas notes "Project 1 - Slides #3.pdf" (pg 6.), but can only handle one pipe. We tried modeling the the second pipe implementation after the notes by adding another array of file descriptors and a second pipe command but it would not output anything.
Second pipe implementation attempt:
![image](https://user-images.githubusercontent.com/79293011/217110493-4edd5322-6dcd-45a6-a491-d6978c891994.png)

We could not get the second pipe to output any code after many hours of debugging so we did not include it into the project.

### Built-In Functions : Exit
Exit works fine with regular commands and IO redirection, however it breaks if a piped command is executed during the shell session. For example, if `ls | wc` is executed, then you need to type `exit` twice into the command line in order to get back to the orginal shell. In the provided picture, in my terminal executable files are highlighted green in the regular shell and are not highlighted when shell.c is running. This is how I keep track of if I am still in the shell implementation or not.

Example picture:
![image](https://user-images.githubusercontent.com/79293011/217136631-8af86660-0f2d-49a6-81c3-1e1794101fdc.png)
