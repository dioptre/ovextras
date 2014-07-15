def setUp(self):
    import os
    ov_binany_path=os.environ['OV_BINARY_PATH']
    self.terminal = App.open("xterm -e " +
             ov_binany_path +"/openvibe-designer.sh --no-session-management --open " +
            ov_binany_path +"/share/openvibe/scenarios/box-tutorials/online-modifiable-temporal-filter.xml")
    while not self.terminal.window():
        wait(1)
    wait("play.png",10)
    
def testChangeParametersOnline(self):
    click("play.png")
    wait("InitialStateSignalDisplay.png",10)
    dragDrop(Pattern("InitialStateSignalDisplay.png").targetOffset(-42,-161), Pattern("InitialStateSignalDisplay.png").targetOffset(-89,194))
    wait(Pattern("Config.png").similar(0.90).targetOffset(43,0),2)
    click(Pattern("Config.png").similar(0.90).targetOffset(43,0))
    wait(Pattern("Filtertype.png").targetOffset(210,0),2)
    click(Pattern("Filtertype.png").targetOffset(210,0))
    type(Key.DOWN)
    type(Key.DOWN)
    type(Key.ENTER)
    wait(Pattern("signalModified.png").similar(0.50),10)
    
    
def tearDown(self):
    mouseMove(Location(0,0))
    if self.terminal.window():
        App.close(self.terminal)
        self.terminal= None

