import json
import logging
import platform
import sys
import time
import pymysql

import greengrasssdk

# Setup logging to stdout
logger = logging.getLogger(__name__)
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

# Creating a greengrass core sdk client
client = greengrasssdk.client("iot-data")

def mysqlconnect(event):
    # To connect MySQL database
    conn = pymysql.connect(
        host='localhost',
        user='lambda', 
        password = 'abc123',
        db='iotdata',
        )

    cur = conn.cursor()
    cur.execute("""insert into records (timestamp, temperature, pressure, Device_ID, humidity) values (%s, %s, %s, %s, %s)""", (event['timestamp'], event['temperature'], event['pressure'], event['Device_ID'], event['humidity']))
    conn.commit()

    # To close the connection
    conn.close()

def lambda_handler(event, context):    
    try:
        mysqlconnect(event)
    except Exception as e:
        logger.error("Failed to store message: " + repr(e))
    return
