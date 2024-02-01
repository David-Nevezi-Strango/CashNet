from suds.client import Client
import requests
import json
# client1 = Client('http://webservices.oorsprong.org/websamples.countryinfo/CountryInfoService.wso?WSDL', cache=None)
# strCurr = "BGU"
#
# responseStr = client1.service.CurrencyName(strCurr)
# print(responseStr.lower())
# client2 = Client('https://www.dataaccess.com/webservicesserver/numberconversion.wso?WSDL', cache=None)
#
#
# if "not found" in responseStr.lower():

#
# print(client2.service.NumberToDollars(10000))
# print(str(client1))
#
# for method in client1.wsdl.services[0].ports[0].methods.values():
#     print('%s' % (method.name))

# client3 = Client("http://www.webservicex.com/CurrencyConvertor.asmx?WSDL", cache=None)
# print(str(client3))
#
# print(client3.service.CelsiusToFahrenheit("69"))


# API_KEY = "c2e3fe930cd14b0cb23076f8d90befeb"
# curr = "%2C".join(["EUR","RON"])
# API_url = 'https://openexchangerates.org/api/latest.json?app_id={api_key}&symbols={curr}&prettyprint=false&show_alternative=false'.format(api_key=API_KEY, curr=curr)
#
# response = requests.post(API_url)
# print(response.json())


# currCheckJson = {
#                     "currency" : "eur"
#                 }
# responseBool = requests.post("https://92egg0rcuj.execute-api.eu-north-1.amazonaws.com/test/checkCurrency", json=currCheckJson).json()["body"]
# print(responseBool)
# print(type(responseBool))
# print(True == responseBool)
print(float("20,2".replace(",",".")))