# -*- coding: utf-8 -*-

from classroom_attendance_system_ui import *
import sys
import numpy as np
import os
import paramiko
from run_system import run_system
import pymysql
import threading
import argparse

# 进程开关标志位
attendance_system_thread = False

def runSystem():
    global attendance_system_thread
    while(attendance_system_thread):
        run_system()

class classroomAttendanceSystem(QtWidgets.QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        super(classroomAttendanceSystem, self).__init__(parent)
        self.setupUi(self)
        self.center()   # 窗口置中
        self.setFixedSize(self.width(), self.height())  # 锁定窗口最大化
        self.slotInit()

    # 界面居中
    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    # 按键槽函数
    def slotInit(self):
        self.buttonStart.clicked.connect(self.excuteAttendanceSystem)
        self.buttonShow.clicked.connect(self.writeAttendanceDatatoTable)

    # 执行考勤系统程序
    def excuteAttendanceSystem(self):
        global attendance_system_thread
        attendance_system_thread = True
        attendanceSystemThread = threading.Thread(target=runSystem)
        attendanceSystemThread.start()
        attendance_system_thread = False
        print('Recognition Done.')

    # 将考勤数据写入表格
    def writeAttendanceDatatoTable(self):
        db = pymysql.connect(host='localhost', user='root', password='fpga6666', database='attendance_system', charset='utf8')
        cursor = db.cursor()

        # 统计文档行数
        txt = open('/home/hiiragiiori/nameList.txt')
        linesNum = 0
        for index, line in enumerate(txt):
            linesNum += 1
        
        # 按行分别存入列表
        txt = open('/home/hiiragiiori/nameList.txt')
        txtList = [line.rstrip('\n') for line in txt]
        
        # 存放文档中的日期、时间和星期
        List = []
        dateList = []
        timeList = []
        weekdayList = []
        nameList = []
        for l in range(int(linesNum)):
            List.append(txtList[l].split())       # 添加日期和时间
        for m in List:
            dateList.append(m[0])
            weekdayList.append(m[1])
            timeList.append(m[2])
            nameList.append(m[3])

        # 存放数据库中的学生信息
        getStudentsInfo = 'select * from students;'
        cursor.execute(getStudentsInfo)
        studentsInfo = cursor.fetchall()
        studentsID = []
        studentsName = []
        studentsSex = []
        for s in studentsInfo:
            studentsID.append(s[0])
            studentsName.append(s[1])
            studentsSex.append((s[2]))

        # 提取课程信息
        courseNameText = self.courseNameEdit.text()
        classroomNumText = self.classroomNumEdit.text()
        
        classroomNumItem = QTableWidgetItem(classroomNumText)
        courseItem = QTableWidgetItem(courseNameText)

        # 写入数据库
        for line in range(linesNum):
            sql = 'insert into attendance \
                  (`DATE`, `WEEKDAY`, `TIME`, `CLASSROOM`, `COURSE`, `ID`, `NAME`, `SEX`) \
                  values \
                  (%s, %s, %s, %s, %s, %s, %s, %s)'
            data = (dateList[line], weekdayList[line], timeList[line], courseNameText, classroomNumText,
                    studentsID[studentsName.index(nameList[line])],
                    nameList[line],
                    studentsSex[studentsName.index(nameList[line])])
            cursor.execute(sql, data)
        db.commit()
        cursor.close()
        db.close()
        print('Database Done')

        # 写入表格
        for idIndex in range(linesNum):
            dateItem = QTableWidgetItem(dateList[idIndex])
            weekdayItem = QTableWidgetItem(weekdayList[idIndex])
            timeItem = QTableWidgetItem(timeList[idIndex])
            classroomNumItem = QTableWidgetItem(classroomNumText)
            courseItem = QTableWidgetItem(courseNameText)
            stuIDItem = QTableWidgetItem(studentsID[studentsName.index(nameList[idIndex])])
            stuNameItem = QTableWidgetItem(studentsName[idIndex])
            stuSexItem = QTableWidgetItem(studentsSex[studentsName.index(nameList[idIndex])])

            self.tableWidget.setItem(idIndex, 0, dateItem)
            self.tableWidget.setItem(idIndex, 1, weekdayItem)
            self.tableWidget.setItem(idIndex, 2, timeItem)
            self.tableWidget.setItem(idIndex, 3, courseItem)
            self.tableWidget.setItem(idIndex, 4, classroomNumItem)
            self.tableWidget.setItem(idIndex, 5, stuIDItem)
            self.tableWidget.setItem(idIndex, 6, stuNameItem)
            self.tableWidget.setItem(idIndex, 7, stuSexItem)

        # 抬头率折线图显示
        if os.path.exists('/home/hiiragiiori/headupList.txt') == True:
            x, y = np.loadtxt('/home/hiiragiiori/headupList.txt', delimiter=' ', unpack=True)
            self.plt.plot(x, y, symbolPen='w', symbolSize='8')

        print('Done.')
    
if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    window = classroomAttendanceSystem()
    window.show()
    sys.exit(app.exec_())
