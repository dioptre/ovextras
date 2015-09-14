def setUp(self):
    import os
    ov_binany_path=os.environ['OV_BINARY_PATH']
    self.terminal = App.open("xterm -e " + ov_binany_path +"/openvibe-designer.sh --no-session-management --play-fast Test_ROCCurve.xml")
    while not self.terminal.window():
        wait(1)
def testROCCurve(self):
    wait("ROCCurveResult.png",10)
    assert(exists("ROCCurveResult.png"))
    
def tearDown(self):
    self.terminal.close()
