import json
from suds.client import Client

def lambda_handler(event, context):
    print("event body: ", event)
    currency = event["currency"]
    print(currency)
    isValid = True
    currChecker = Client('http://webservices.oorsprong.org/websamples.countryinfo/CountryInfoService.wso?WSDL', cache=None)
    responseStr = currChecker.service.CurrencyName(currency.upper())
    print(responseStr)
    if "not found" in responseStr.lower():
        isValid = False
        
    return {
        'statusCode': 200,
        'body': isValid
    }
