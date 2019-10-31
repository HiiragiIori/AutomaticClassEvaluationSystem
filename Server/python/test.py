import pymysql

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
print(dateList)
print(weekdayList)
print(timeList)
print(nameList)

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
print(studentsID)
print(studentsName)
print(studentsSex)
