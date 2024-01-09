# FAT12-Project
Assignment for Operating Systems (Fall 2022) at University of Victoria

To compile and run the code: <br>
    From UVic's Linux Environment, compile code by executing "make" from terminal while in the directory containing assignment files.  
    Running "make" will create four executables; diskinfo, disklist, diskget and diskput.

1. To run diskinfo, execute "./diskinfo <file system image name>" in terminal
        This will display information about the file system  including:
        OS Name, Disk Label, Total Size of Disk, Free Size of Disk, Number of Files in Disk,
        Number of FAT Copies and Sectors per FAT
    
2. To run disklist, execute "./disklist <file system image name>" in terminal
        This will display the contents of the root directory and all sub-directories 
        in the file system.

3. To run diskget, execute "./diskget <file system image name> <file name>" in terminal
        This will copy the inputted file from the root directory of the file system to your current
        Linix directory.
    
4. To run diskput, execute "./diskput <file system image name> <file path>" in terminal
        This will copy the inputted file from your current linux directory into the specificied 
        directory of the file sytem. File path inputs should follow this formatting:
        For subdirectories: /sub1/sub2/sub3/foo.txt where sub1 is a sub-directory of the 
        root directory and sub2 is a sub-directory of sub1, and foo.txt is the file name.
        For root directory: foo.txt where foo.txt is the file name.

<br>NOTE: The file name is case sensitive to how it is written in the Linux directory.


