def setUp(self):
    import os
    import shutil
    ov_binany_path=os.environ['OV_BINARY_PATH']
    self.terminal = App.open("xterm -e " + ov_binany_path +"/openvibe-designer.sh --no-session-management --play-fast Test_ROCCurve.xml")
    while not self.terminal.window():
        wait(1)
        
 #def takepicture:
  
  #dir = os.path.dirname(getBundlePath()) # the folder, where your script is stored
  #img = capture(SCREEN) # snapshots the screen
  #shutil.move(img, os.path.join(dir, "shot.png")) 
  
def testROCCurve(self):
    import os
    import shutil
    try:
      wait("ROCCurveResult.png",60)
      assert(exists("ROCCurveResult.png"))
    except (FindFailed, AssertionError):
      print "Unable to find the required png"
      dir = os.path.dirname(getBundlePath()) # the folder, where your script is stored
      img = capture(SCREEN) # snapshots the screen
      shutil.move(img, os.path.join(dir, "screenshot.png")) 
      raise
    
def tearDown(self):
    self.terminal.close()
