# Moolticute Device Emulation Mode

In order to test the Mooltipass ecosystem, Moolticute integrates a feature that simulates a Mooltipass device and its basic credential storage and recall functionalities.  
This feature can be enabled by launching the Moolticute with the "-e" parameter.  

# Windows Guide  
  
## Step 1: Close Moolticute  

Right click on the Moolticute icon, click on "Quit".  
<p align="center">
  <img src="https://raw.githubusercontent.com/mooltipass/moolticute/master/documentation/ressources/close_mc.png"/>
</p>
  
  
## Step 2: Start Moolticute Daemon in Emulation Mode  

Start a command prompt, navigate to the C:\Users\<your user name>\AppData\Local\Programs\Moolticute directory and type "moolticuted.exe -e":  
<p align="center">
  <img src="https://raw.githubusercontent.com/mooltipass/moolticute/master/documentation/ressources/mc_emulation_launch.png"/>
</p>
  
  
## Step 3: Launch Moolticute   

Either type "moolticute.exe" in the command prompt in Step 2, or use Windows menu to start moolticute.
  
  
# Analyzing the Daemon Logs  

## Accessing the Logs

Click the moolticute icon to bring up its window, select the settings tab and click on "View" next to "View Daemon Logs":  
<p align="center">
  <img src="https://raw.githubusercontent.com/mooltipass/moolticute/master/documentation/ressources/logs_view.png"/>
</p>
  
  
## Checking for Credentials Storage

<p align="center">
  <img src="https://raw.githubusercontent.com/mooltipass/moolticute/master/documentation/ressources/mc_storage.png"/>
</p>
  
  
## Checking for Credentials Recall

<p align="center">
  <img src="https://raw.githubusercontent.com/mooltipass/moolticute/master/documentation/ressources/mc_recall.png"/>
</p>

  
