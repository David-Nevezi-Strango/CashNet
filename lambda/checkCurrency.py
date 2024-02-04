import json
import requests
API_URL = 'https://openexchangerates.org/api/latest.json?app_id={api_key}&symbols={curr}&prettyprint=false&show_alternative=false'
from keys import API_KEY

def lambda_handler(event, context):
    print("event body: ", event)
    src_currency = event["src_currency"]
    dest_currency = event["dest_currency"]
    curr = "%2C".join([src_currency, dest_currency])
    exchangeRateResponse = requests.post(API_URL.format(api_key=API_KEY, curr=curr)).json()["rates"]
    exchangeRate = (1/exchangeRateResponse[src_currency]) * exchangeRateResponse[dest_currency]
                    
        
    return {
        'statusCode': 200,
        'exchangeRate': exchangeRate
    }
