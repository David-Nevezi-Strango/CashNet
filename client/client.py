from utils import console, communicate, createSocket, sha_256, CLEAR_CMD, DIGIT_REGEX
import getpass
import os
import datetime
from suds.client import Client
import requests
customer = {}

def createAccount():
    #function to create a new account
    global customer
    os.system(CLEAR_CMD)
    createText="""
        What currency would you like to have the account in? 
        Please type the international abreviation for it(or press ENTER to return)
            """
    currency = input(createText).strip()
    #if enter was pressed, exit from this function
    if currency == "":
        return
    else:
        while currency != "":
            found = False
            #check if an account already exists with the specified currency
            if customer["accounts"]:
                for account in customer["accounts"]:
                    if account["currency"] == currency.upper():
                        found = True
                        break
            if found:
                currency = input("\tSorry, but you cannot have multiple accounts of the same currency!\n\tPlease select other currencies or press ENTER to return\n\t\t").strip()
            else:
                # check if the currency was typed in correctly
                currChecker = Client('http://webservices.oorsprong.org/websamples.countryinfo/CountryInfoService.wso?WSDL', cache=None)
                responseStr = currChecker.service.CurrencyName(currency[:3].upper())
                if "not found" in responseStr.lower():
                    currency = input(
                        "\tSorry, but you have typed in an invalid currency!\n\tPlease select other currencies or type press ENTER to return\n\t\t")
                    continue
                # currCheckJson = {
                #     "currency" : currency
                # }
                # responseJSON = requests.post("https://92egg0rcuj.execute-api.eu-north-1.amazonaws.com/test/checkCurrency", json=currCheckJson).json()
                # responseBool = responseJSON["body"]
                # if not responseBool:
                #     currency = input(
                #         "\tSorry, but you have typed in an invalid currency!\n\tPlease select other currencies or type press ENTER to return\n\t\t")
                #     continue
                #create the account  
                requestJson = {
                    "call" : "postAccount",
                    "customer_id" : customer["id"],
                    "current_sum" : 0.0,
                    "currency" : currency.upper(),
                    "date" : datetime.datetime.now().strftime('%Y %b %d %H:%M:%S %Z%z')
                }
                resp = communicate("request", requestJson)
                if "done" in resp:
                    print("\tCreation successful! Press ENTER to return")
                    input()
                    break
                else:
                    print("\t\tCreation failed! Press ENTER to return")
                    input()
                    break

def closeAccount():
    #function to close an existing account
    global customer
    os.system(CLEAR_CMD)
    accountIdx = accountSelection()
    if accountIdx == "":
        return
    if customer["accounts"][accountIdx]["current_sum"] != 0:
        print("\tSorry, but you cannot close an account if you have a deposited sum. Please withdraw it before closure\n\tPress ENTER to return\n\t\t")
        input()
        return
    requestJson = {
        "customer_id" : customer["id"],
        "call" : "postTransaction",
        "account_id" : customer["accounts"][accountIdx]["account_id"],
        "transaction_type" : 1,
        "dest_account_id" : -1,
        "date" : datetime.datetime.now().strftime('%Y %b %d %H:%M:%S %Z%z'),
        "sum" : 0.0
    }
    resp = communicate("request", requestJson)
    if "done" in resp:
        print("\t\tClosure  successful! Press ENTER to return")
        input()
    else:
        print("\t\tClosure failed!  Press ENTER to return")
        input()

def accountSelection():
    #helper function to select an account from a list
    global customer
    accountText = """
        Please select the account (the number after #) or press ENTER to return:
        
        {accounts}
    \t"""
    accounts_str = ""
    account_nr = 1
    for account in customer["accounts"]:
        accounts_str += "Account #" + str(account_nr) + ": ID:  " + str(account["account_id"]) + " - " + str(round(account["current_sum"], 2)) + " " + account["currency"] + "\n\t"
        account_nr += 1
    keyboard_input = console(accountText.format(accounts=accounts_str), len(customer["accounts"]))
    #exception handling
    #if enter was pressed, exit from this function
    if keyboard_input == "":
        return keyboard_input
    return keyboard_input - 1

def withdraw(transactionFlag = False):
    #function to withdraw or transfer a sum from an account
    global customer
    os.system(CLEAR_CMD)
    accountIdx = accountSelection()
    if accountIdx == "":
        return
    withdrawText = """
        Please type in the amount of money to be {transaction_type} (or press ENTER to return):
    \t"""
    print(withdrawText.format(transaction_type="withdrawn" if not transactionFlag else "transferred"))
    sum = input("\t\tSum: ").strip()
    #if enter was pressed, exit from this function
    if sum == "":
        return
    #exception handling
    if sum != "" and DIGIT_REGEX.match(sum):
        sum = float(sum.replace(",","."))
        while sum < 0 or customer["accounts"][accountIdx]["current_sum"] - sum < 0:
            sum = input("\t\tInsufficient funds! Please type in a smaller sum(or press ENTER to return):").strip()
            #if enter was pressed, exit from this function
            if sum == "":
                return
            #exception handling
            elif sum != "" and DIGIT_REGEX.match(sum):
                sum = float(sum.replace(",","."))
        else:
            if transactionFlag:
                #if it is a transaction, calculate the exchange rate 
                dest_account = input("\t\tPlease type in the destination account's ID: ").strip()
                #if enter was pressed, exit from this function
                if dest_account == "":
                    return
                #exception handling
                if dest_account != "" and DIGIT_REGEX.match(dest_account):
                    dest_account = int(dest_account)
                    if dest_account ==  customer["accounts"][accountIdx]["account_id"]:
                        print("\t\t{transaction_type} failed because of destination account (It is the same as the source account)! Press ENTER to return".format(transaction_type="Withdraw" if not transactionFlag else "Transfer"))
                        input()
                        return
                #get destination account's currency
                destCurrencyRequest = {"customer_id" : customer["id"],"call" : "getAccountByID", "account_id": dest_account}
                destCurrencyResponse = communicate("request", destCurrencyRequest)
                if not destCurrencyResponse:
                    print("\t\t{transaction_type} failed because of destination account (It might be closed)! Press ENTER to return".format(transaction_type="Withdraw" if not transactionFlag else "Transfer"))
                    input()
                    return
                src_currency = customer["accounts"][accountIdx]["currency"]
                dest_currency = destCurrencyResponse["response"]["currency"]
                #calculate exchange rate
                if src_currency == dest_currency:
                    exchangeRate = 1
                else:
                    currCheckJson = {
                        "src_currency" : src_currency,
                        "dest_currency" : dest_currency
                    }
                    responseJSON = requests.post("https://92egg0rcuj.execute-api.eu-north-1.amazonaws.com/test/checkCurrency", json=currCheckJson).json()
                    exchangeRate = responseJSON["exchangeRate"]
                    # curr = "%2C".join([src_currency, dest_currency])
                    # exchangeRateResponse = requests.post(API_URL.format(api_key=API_KEY, curr=curr)).json()["rates"]
                    # exchangeRate = (1/exchangeRateResponse[src_currency]) * exchangeRateResponse[dest_currrency]
                    
                    
                    print("\t\tThe exchange rate being used is the following: {rate}\n".format(rate=round(exchangeRate, 2)))
                converted_sum = round(sum * exchangeRate, 2)
            else:
                #if it is a withdraw, put dummy data
                dest_account = -1
                converted_sum = None

            requestJson = {
                "customer_id" : customer["id"],
                "call" : "postTransaction",
                "account_id" : customer["accounts"][accountIdx]["account_id"],
                "transaction_type" : 4 if transactionFlag else 2,
                "dest_account_id" : dest_account,
                "date" : datetime.datetime.now().strftime('%Y %b %d %H:%M:%S %Z%z'),
                "sum" : sum,
                "dest_sum" : converted_sum
            }
            resp = communicate("request", requestJson)
            if "done" in resp:
                print("\t\t{transaction_type} successful! Press ENTER to return".format(transaction_type="Withdraw" if not transactionFlag else "Transfer"))
                input()
            else:
                print("\t\t{transaction_type} failed! Press ENTER to return".format(transaction_type="Withdraw" if not transactionFlag else "Transfer"))
                input()
    else:
        print("\tInvalid sum! Please re-try with a valid sum (press ENTER to return)")                
        input()

def deposit():
    #function to deposit into an account
    global customer
    os.system(CLEAR_CMD)
    accountIdx = accountSelection()
    if accountIdx == "":
        return
    depositText = """
        Please type in the amount of money to be deposited (or press ENTER to return):\n
    \t"""
    print(depositText)
    sum = -1
    while sum < 0 or customer["accounts"][accountIdx]["current_sum"] - sum < 0:
        sum = input("\tSum: ").strip()
        #if enter was pressed, exit from this function
        if sum == "":
            return
        #exception handling
        if sum != "" and DIGIT_REGEX.match(sum):
            sum = float(sum.replace(",","."))
        else:
            sum = -1
            print("\tInvalid sum! Please type in a valid sum (or press ENTER to return)")
    else:
        requestJson = {
            "customer_id" : customer["id"],
            "call" : "postTransaction",
            "account_id" : customer["accounts"][accountIdx]["account_id"],
            "transaction_type" : 3,
            "dest_account_id" : -1,
            "date" : datetime.datetime.now().strftime('%Y %b %d %H:%M:%S %Z%z'),
            "sum" : sum
        }
        resp = communicate("request", requestJson)
        if "done" in resp:
            print("\t\tDeposit successful! Press ENTER to return")
            input()
            return
        else:
            print("\t\tDeposit failed! Press ENTER to return")
            input()
            return


def transaction():
    withdraw(True)

def viewHistory():
    #function to visualize transaction history of an account
    global customer
    os.system(CLEAR_CMD)
    accountIdx = accountSelection()
    if accountIdx == "":
        return
    historyText = """
        Here is your account's history (Press ENTER to return):
        
    {transactions}
    """
    requestJson = {
        "customer_id" : customer["id"],
        "account_id" : customer["accounts"][accountIdx]["account_id"],
        "call" : "getAccountTransactions"
    }
    resp = communicate("request", requestJson)
    #exception handling
    if type(resp) == dict and "response" in resp.keys():
        resp = resp["response"]
    #prepare the string of the history
    transaction_str = ""
    for transaction in resp:
        transaction_type = transaction["transaction_type"]
        if transaction_type == 0:
            transaction_type = "Account created"
            transaction["dest_account_id"] = ""
        elif transaction_type == 1:
            transaction_type = "Account closed"
            transaction["dest_account_id"] = ""
        elif transaction_type == 2:
            transaction_type = "Withdraw"
            transaction["dest_account_id"] = ""
        elif transaction_type == 3:
            transaction_type = "Deposit"
            transaction["dest_account_id"] = ""
        elif transaction_type == 4:
            transaction_type = "Transaction sent"
        elif transaction_type == 5:
            transaction_type = "Transaction received"
        transaction_str += "\t\t" + transaction_type
        transaction_str += ", Sum: " + str(transaction["sum"]) + " " + customer["accounts"][accountIdx]["currency"] if transaction["transaction_type"] not in [0, 1] else ""
        transaction_str += " - " + transaction["date"]
        if transaction["dest_account_id"] != "":
            transaction_str += "; Destination account ID: " + str(transaction["dest_account_id"])
        transaction_str += "\n"
    print(historyText.format(transactions=transaction_str))
    input()

def mainMenu():
    global customer
    requestJson = {
    "customer_id": customer["id"],
    "call": "getCustomerAccounts"}

    menuText = """
        Welcome!
        
        Here are the details of your accounts: 
        
        {accounts}
        
        What would you like to do?
        1. Create new account
        2. Close existing account
        3. Withdraw
        4. Deposit
        5. Make Transaction
        6. View Transaction History
        7. Log out
    \t"""
    option = 0
    while option != 7:
        resp = communicate("request", requestJson)
        customer["accounts"] = resp["response"]
        account_str = ""
        account_nr = 1
        if customer["accounts"]:
            for account in customer["accounts"]:
                account_str += "Account #" + str(account_nr) + ": ID:  " + str(account["account_id"]) + " - " + str(round(account["current_sum"], 2)) + " " + account["currency"] + "\n\t"
                account_nr += 1
        else:
            account_str = "None"
        os.system(CLEAR_CMD)
        option = console(menuText.format(accounts=account_str), 7)

        if option == 1:
            createAccount()
        elif option == 2:
            closeAccount()
        elif option == 3:
            withdraw()
        elif option == 4:
            deposit()
        elif option == 5:
            transaction()
        elif option == 6:
            viewHistory()
    else:
        logout()

def logout():
    communicate("exit")
    exit()


def login():
    global customer
    global TCPSocket
    os.system(CLEAR_CMD)
    loginText = """
        Please log in!
        
        Name:"""
    passwordText = "\tPassword: "

    createSocket()

    ok = False
    while not ok:
        name = input(loginText).strip()
        password = getpass.getpass(passwordText).strip()
        passwordEncrypted = sha_256(password)
        requestJson = {
        "customer_id": -1,
        "call": "loginCustomer",
        "customer_name": name,
        "customer_password": passwordEncrypted}
        resp = communicate("request", requestJson)
        if resp["id"] != -1:
            customer["customer_name"] = name
            customer["id"] = resp["id"]
            ok = True
        else:
            os.system(CLEAR_CMD)
            print("\tWrong credentials!")

    if ok:
        mainMenu()

login()