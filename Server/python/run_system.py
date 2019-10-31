import pymysql
import os

def run_system():
    
    # 运行TCP/IP接收端程序
    val1 = os.system('cd ../ && ./server')
    print(val1)

    # 运行识别程序
    val2 = os.system('cd ../../my_build/intel64/Release && ./attendance_system_images fpga')
    print(val2)

if __name__ == '__main__':
    run_system()
