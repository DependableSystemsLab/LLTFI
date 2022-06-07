## Dependencies: ##
Nodejs.
webpack   

## Steps to set up the development environment: ##
1: Download this project from Git   
2: Download NodeJs   
3: Install libraries: Go to the web-app directory and run "npm install". The required versions can be found in package.json file.   
4: Install Webpack: In the same directory as step 3, run "sudo npm install -g webpack".   
5: Configurate the LLFI root path for the server:   
The default bevaiour of the program use environment variable `$llfibuild` as the path of the llfi build directory  
You can set the environment variable `$llfibuild` in your system to point it to the LLFI build directory in your local machine.   

Start the server:   
Go to the /web-app/server folder and run "node server.js"  

Start the front-end dev tool:   
Go to the web-app directory and run "webpack" or "webpack -w"   



## Overview of the Web Application ## 

Refernce: * [LLFI Web GUI](https://github.com/DependableSystemsLab/LLFI/wiki/Get-Started-with-using-LLFI-with-Web-GUI)

Here we describe the procedure for performing fault injections in LLTFI using the Web GUI. We provide step-by-step instructions to inject fault and to view the results. The process differs slightly for Hardware/Software Injection and the differences are described below.

### Fault Injection Procedure using the GUI ###

The first step is to select the target program for fault injection. The user can upload any standalone C/C++ source file by selecting; File->Open File.

The figure below is a screenshot for opening a single file.

![Alt text](images/fileUpload.png?raw=true)

The procedure contains the following steps once the file or project is uploaded:

1) Compile To IR: If a C/C++ source file or project is uploaded then click on the "Compile To IR" button. When the program is successfully compiled, a success message with the name of IR file that is created will be displayed. The figure below is a screenshot of a successful completion of "Compile To IR".

![Alt text](images/CompileToIR.png?raw=true)

2) Instrument: Once the intermediate representation (IR) file is created click on "Instrument" button. The instrument dialog box will open, where the user can select the configuration parameters (input.yaml).

For Hardware Faults Injection, select "Hardware Injection", then select the Instruction Type, Register Location and trace options before clicking the "Instrument" button in the dialog box. The figure below is an example of the Hardware Faults Instrument interface.

![Alt text](images/Instrument.png?raw=true)

<!--For Software Failures Injection, select "Software Injection", then select one of the failures types and trace options before clicking the "Instrument" button in the dialog box. Available software failures are scanned using Software Failure Auto Scan. If more than one software failures are selected, only the first selection will be used. The figure below is an example of the Software Failures Instrument interface.

Instrument-software######### -->

3) Profiling:After instrumentation, click on the "Profiling" button. However if the program requires any command line inputs, provide them in the "Command Line Input" text box before clicking "Profiling". Profiling displays the No.of.Index and No.of.Cycle on the Profiling Status tab in the bottom pane.

The figure below is a successful completion of hardware faults profiling.

![Alt text](images/Profiling.png?raw=true)

4) Runtime Options: Click on the "Runtime Options" to configure the fault injection. The user can also save the profile by checking the "Save Profile" checkbox before clicking "Submit". If the user uploaded a .yaml file with the runtime options then this step could be skipped. Click on the "Delete Run" button to delete a runtime option.

For Hardware Faults Injection, the user should provide Number of Runs and Fault injection type (mandatory), while other inputs are optional. The figure below is a screenshot of the Hardware Runtime Options interface.

![Alt text](images/RuntimeOptions.png?raw=true)

<!-- For Software Failures Injection, the user should provide Number of Runs (mandatory), while other inputs are optional. The figure below is a screenshot of the Single Software Runtime Options interface.

RuntimeOption-software########## -->

5) Fault Injection: Click on the "Inject Fault" button to perform fault injection. When fault injection is completed click on the "Fault Injection Status" and "Fault Summary" tabs to view the result of the fault injections.

The figure below is a screenshot of a completion of a Hardware "Inject Fault", you can also see the "Fault Injection Status" table in the figure.

![Alt text](images/InjectFault.png?raw=true)
![Alt text](images/InjectFault2.png?raw=true)

6) Trace Graph: Select one or more traces from the "Fault Injection Status" tabs. Click on the "Trace Graph" button and a trace graph window will pop up. The trace graph contains the flow of the LLVM IR instructions, and the mapping information of LLVM IR instructions to the C source code (if applicable). The trace graph will be downloaded in a pdf format.

![Alt text](images/TraceGraph.png?raw=true)

*SDC Option*: By default the SDC's are reported by making a diff between the std_outputfile and the golden_output. The user can customize the way SDC are generated by writing a script and provide the path of the script to the environment variable COMPARE.



