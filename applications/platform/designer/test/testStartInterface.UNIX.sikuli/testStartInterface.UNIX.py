def setUp(self):
    import os
    ov_binany_path=os.environ['OV_BINARY_PATH']
    self.terminal = App.open("xterm")
    while not self.terminal.window():
        wait(1)
    type(ov_binany_path +"/openvibe-designer.sh --no-session-management"+ Key.ENTER)
    wait("StartInterface.png",10)
def test_createSimpleScenarioAndRun(self):
    click(Pattern("Datagenerati.png").targetOffset(-70,-1))
    dragDrop("Sinusoscilla-1.png",Pattern("EEKLBTime001.png").similar(0.19).targetOffset(-233,-163))
    assert(exists("SinusOscillatorBoxSelected.png"))
    click(Pattern("Visualisatio.png").targetOffset(-62,0))
    click(Pattern("Basic.png").targetOffset(-50,-1))
    dragDrop("ESignaldispl.png", Pattern("itENEExlJEll.png").similar(0.16).targetOffset(-288,-26))
    assert(exists("SignalDisplayBoxSelected.png"))
    dragDrop("outputSignalConnector.png", "imputSingnalConnector.png")
    click(Pattern("playButton.png").similar(0.95))
    wait(6)
    assert(exists(Pattern("SignalDisplayWindow.png").similar(0.58)))
    click("stopButton.png")
    waitVanish(Pattern("SignalDisplayWindow.png").similar(0.58))
             
def tearDown(self):
    App.close(self.terminal)

