# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'classroom_attendance_system_ui.ui'
#
# Created by: PyQt5 UI code generator 5.13.0
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import *
from PyQt5.QtCore import QThread, QDateTime, QDate
import pyqtgraph as pg
from pyqtgraph.Qt import QtGui, QtCore

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(1920, 1080)
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        MainWindow.setFont(font)

        self.centralwidget = QtWidgets.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        
        # 文本框文字说明--------------------------------------------------------------------
        self.courseNameLabel = QtWidgets.QLabel(self.centralwidget)
        self.courseNameLabel.setObjectName("courseNameLabel")
        self.courseNameLabel.setText('课程名称')
        self.courseNameLabel.setGeometry(QtCore.QRect(30, 30, 80, 30))
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        font.setPointSize(12)
        self.courseNameLabel.setFont(font)
        self.classroomNumLabel = QtWidgets.QLabel(self.centralwidget)
        self.classroomNumLabel.setObjectName("classroomNumLabel")
        self.classroomNumLabel.setText('教室')
        self.classroomNumLabel.setGeometry(QtCore.QRect(250, 30, 60, 30))
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        font.setPointSize(12)
        self.classroomNumLabel.setFont(font)
        # 文本框文字说明--------------------------------------------------------------------

        # 文本框输入------------------------------------------------------------------------
        self.courseNameEdit = QtWidgets.QLineEdit(self.centralwidget)
        self.courseNameEdit.setGeometry(QtCore.QRect(100, 30, 100, 30))
        self.courseNameEdit.setObjectName("CourseNameEdit")
        self.classroomNumEdit = QtWidgets.QLineEdit(self.centralwidget)
        self.classroomNumEdit.setGeometry(QtCore.QRect(310, 30, 100, 30))
        self.classroomNumEdit.setObjectName("ClassroomNumEdit")
        # 文本框输入------------------------------------------------------------------------

        # 学生名单--------------------------------------------------------------------------
        self.tableWidget = QtWidgets.QTableWidget(self.centralwidget)
        self.tableWidget.setGeometry(QtCore.QRect(800, 40, 1020, 450))
        self.tableWidget.setRowCount(100)
        self.tableWidget.setColumnCount(8)
        self.tableWidget.setObjectName("tableWidget")
        self.tableWidget.horizontalHeader().setDefaultSectionSize(120)
        self.tableWidget.setHorizontalHeaderLabels(['日期', '星期', '时间', '课程', '教室', '学号', '学生', '性别'])
        # 学生名单--------------------------------------------------------------------------

        # 按键------------------------------------------------------------------------------
        self.buttonStart = QtWidgets.QPushButton(self.centralwidget)
        self.buttonStart.setGeometry(QtCore.QRect(210, 100, 150, 150))
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        font.setPointSize(12)
        self.buttonStart.setFont(font)
        self.buttonStart.setObjectName("buttonStart")
        self.buttonStart.setDefault(True)
        self.buttonShow = QtWidgets.QPushButton(self.centralwidget)
        self.buttonShow.setGeometry(QtCore.QRect(210, 300, 150, 150))
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        font.setPointSize(12)
        self.buttonShow.setFont(font)
        self.buttonShow.setObjectName("buttonShow")
        # 按键------------------------------------------------------------------------------

        # 抬头率折线图-----------------------------------------------------------------------
        self.verticalLayoutWidget = QtWidgets.QWidget(self.centralwidget)
        self.verticalLayoutWidget.setGeometry(QtCore.QRect(800, 500, 1020, 450))
        self.verticalLayoutWidget.setObjectName("verticalLayoutWidget")
        self.verticalLayout = QtWidgets.QGridLayout(self.verticalLayoutWidget)
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout.setObjectName("verticalLayout")
        self.plt = pg.PlotWidget(title='抬头率折线图')
        self.plt.showGrid(x=True, y=True)   # 显示图形网格
        self.plt.setXRange(0, 45)
        self.plt.setYRange(0, 100)
        self.plt.setLabel('left', '抬头率', units='%')
        self.plt.setLabel('bottom', '时间', units='s')
        self.verticalLayout.addWidget(self.plt)
        # 抬头率折线图-----------------------------------------------------------------------

        # 实时时间显示-----------------------------------------------------------------------
        self.timeLabel = QtWidgets.QLabel(self)
        self.timeLabel.setFixedWidth(300)
        self.timeLabel.move(20, 900)
        timer = QtCore.QTimer(self)
        timer.timeout.connect(self.showTime)
        timer.start(1)
        # 实时时间显示-----------------------------------------------------------------------

        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtWidgets.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1280, 23))
        self.menubar.setObjectName("menubar")
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QtWidgets.QStatusBar(MainWindow)
        self.statusbar.setObjectName("statusbar")
        MainWindow.setStatusBar(self.statusbar)

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    # 实时时间函数
    def showTime(self):
        time = QDateTime.currentDateTime()
        date = QDate.currentDate()
        text = date.toString('yyyy年MM月dd日') + '  ' + time.toString('hh:mm:ss')
        self.timeLabel.setText(text)
        font = QtGui.QFont()
        font.setFamily("微软雅黑")
        font.setPointSize(12)
        self.timeLabel.setFont(font)

    def retranslateUi(self, MainWindow):
        _translate = QtCore.QCoreApplication.translate
        MainWindow.setWindowTitle(_translate("MainWindow", "课堂评价系统"))
        self.buttonStart.setText(_translate("MainWindow", "开始考勤"))
        self.buttonShow.setText(_translate("MainWindow", "显示结果"))
        # self.buttonExport.setText(_translate("MainWindow", "导出为文本"))
