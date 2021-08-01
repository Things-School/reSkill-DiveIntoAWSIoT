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

def lambda_handler(event, context):    
    try:
        event["timestamp"] = event["timestamp"] + 946684800
        client.publish(
            topic="",
            queueFullPolicy="AllOrException",
            payload=json.dumps(event)
        )
    except Exception as e:
        logger.error("Failed to publish message: " + repr(e))
    return
