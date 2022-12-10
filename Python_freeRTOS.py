from PyQt5 import QtWidgets, uic
#from PyQt5.QtWidgets import
from PyQt5.QtWidgets import QSlider, QLCDNumber
from pyqtgraph import PlotWidget
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtGui
import sys
import numpy as np
import serial
from pyqtgraph.ptime import time
import time
import struct

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)
        #Load the UI Page
        uic.loadUi('ventanaqt.ui', self)

        self.stm = serial.Serial('com7', 115200)

        pen = pg.mkPen(color=(255, 0, 0))
        pen2 = pg.mkPen(color=(0, 255, 0))

        self.data1 = np.zeros(2000)
        self.graphWidget.setBackground('w')
        self.g_Layout = self.graphWidget

        self.p1 = self.g_Layout.addPlot()
        self.curve1 = self.p1.plot(self.data1, pen = pen)

        self.data2 = np.zeros(2000)
        self.curve2 = self.p1.plot(self.data2, pen =  pen2)

        self.slider = self.findChild(QSlider,"SliderWidget")
        self.slider2 = self.findChild(QSlider,"horizontalSlider_2")

        #LCD display
        self.lcd = self.findChild(QLCDNumber, "lcdNumber")
        self.lcd_2 = self.findChild(QLCDNumber, "lcdNumber_2")

        timer = QtCore.QTimer(self)
        timer.timeout.connect(self.update)
        timer.start(5)

    # update all plots
    def update(self):
        self.update1()
        self.update2()
        self.updateSlider()

    def update1(self):
        self.data1[:-1] = self.data1[1:]
        a = self.stm.readline()

        if(int(a[0]) == 65):
            a1 = int.from_bytes(a[1:4], byteorder = 'little')
            x1 = (int(a1)*3.3)/4096
            self.data1[-1] = x1
            self.curve1.setData(self.data1)

    def update2(self):
        self.data2[:-1] = self.data2[1:]
        a = self.stm.readline()
        #print(a)

        if(int(a[0]) == 66):
            a1 = int.from_bytes(a[1:4], byteorder = 'little')
            x1 = (int(a1)*3.3)/4096
            self.data2[-1] = x1
            self.curve2.setData(self.data2)

    def updateSlider(self):
        self.lcd.display(self.slider.value())
        self.lcd_2.display(self.slider2.value())

        result = self.slider.valueChanged[int]
        bytes_res = result.to_bytes(1, 'little')
        #self.stm.write(bytes_res)
        #print("Slider 1: ", bytes_res)

        result2 = self.slider2.value()
        #bytes_sum = sumResult.to_bytes(4, 'little')
        bytes_res2 = result2.to_bytes(1, 'little')

        bytes_sum = bytes_res + bytes_res2

        #old_bytes = bytes_sum

        #print("Slider 2: ", bytes_res2)
        if (old_bytes != bytes_sum):
            print("Slider Sum: ", bytes_sum)
            self.stm.write(bytes_sum)

def main():
    app = QtWidgets.QApplication(sys.argv)
    main = MainWindow()
    #main.showFullScreen()
    main.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
