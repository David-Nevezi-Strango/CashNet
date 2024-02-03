from utils import console, communicate, createSocket, sha_256, CLEAR_CMD, DIGIT_REGEX
import os
import datetime

def getSocketsList():
    #function to get a list of all connections in list and str form
    requestJson = {
        "call" : "listsockets"
    }
    resp = communicate("request", requestJson)
    #exception handling
    if type(resp) == dict and "response" in resp.keys():
        resp = resp["response"]
    sockets_list = resp
    list_str = ""
    #create str of the list
    if sockets_list:
        for pair in sockets_list:
            list_str = "Customer ID: " + str(pair["ID"]) + " Thread ID: " + str(pair["thread_id"]) + "\n\t"
    else:
        list_str = "None"
    return list_str, sockets_list

def listAll():
    #function to print all connection
    os.system(CLEAR_CMD)
    listText = """
        Here are all the connected clients (or press ENTER to return):
        
        {list}
    \t"""

    option = input(listText.format(list=getSocketsList()[0])).strip()
    while option != "":
        trial = input("\t\tYou have typed in an invalid character, please try again!\n\t").strip()
        option = trial

def killSocket():
    #function to kill a connection
    os.system(CLEAR_CMD)
    listText = """
        Here are all the connected clients:
        
        {list}
        
        Please type in the customer ID which you would like to cut it's connection (or press ENTER to return):
    \t"""
    list_str, sockets_list = getSocketsList()
    #print the list of connection
    trial = input(listText.format(list=list_str)).strip()
    option = -1
    while option != "":
        #if enter was pressed, exit from this function
        if trial == "":
            option = ""
            continue
        #exception handling
        elif trial != "" and DIGIT_REGEX.match(trial):
            option = int(trial)
        else:
            trial = input("\t\tYou have typed in an invalid option, please try again!\n\t").strip()
            continue
        found = False
        thread_id = None
        # get thread id associated with the customer id given by admin
        for pair in sockets_list:
            if pair["ID"] == option:
                found = True
                thread_id = pair["thread_id"]
                break
        if found:
            requestJson = {
                "call" : "kill",
                "thread_id" : thread_id
            }
            resp = communicate("request", requestJson)
            if "done" in resp:
                print("\t\tKill successful! Press ENTER to go back")
                input()
                break
            else:
                print("\t\tKill failed! Please try again or press ENTER to go back!")
                # input()
                # break
        else:
            trial = input("\t\tYou have typed in an invalid option, please try again!\n\t")


def createCustomer():
    #function to create a customer
    os.system(CLEAR_CMD)
    nameText = "\t\tPlease type in the customer's name (or press ENTER to go back): "
    passwordText = "\t\tPlease type in the customer's password(or press ENTER to go back): "
    option = None
    while option != "":
        name = input(nameText).strip()
        #if enter was pressed, exit from this function
        if name == "":
            return
        password = input(passwordText).strip()
        if password == "":
            return
        requestJson = {
            "call" : "postCustomer",
            "customer_name" : name,
            "customer_password" : sha_256(password)
        }
        resp = communicate("request", requestJson)
        if "done" in resp:
            print("\t\tCreation successful! Press ENTER to go back")
            input()
            break
        else:
            print("\t\tCreation failed! Press ENTER to go back")
            option = input()

def connectCustomerToAccount():
    #function to connect a customer to an account
    os.system(CLEAR_CMD)
    requestCustomerJson = {"call" : "getCustomers"}
    requestAccountJson = {"call" : "getAccounts"}
    customers = communicate("request", requestCustomerJson)
    #exception handling
    if type(customers) == dict and "response" in customers.keys():
        customers = customers["response"]
    accounts = communicate("request", requestAccountJson)
    if type(accounts) == dict and "response" in accounts.keys():
        accounts = accounts["response"]
    customerText = "{customer_list}\n\t\tPlease type in the customer's ID(or press ENTER to go back): "
    accountText = "{account_list}\n\t\tPlease type in the account's ID(or press ENTER to go back): "
    customers_str = ""
    #create str that will be output to the client console
    if customers:
        for customer in customers:
            customers_str += "\t\tCustomer: " + customer["customer_name"] + ", ID: " + str(customer["customer_id"]) + "\n"
    else:
        customers_str = "None"
    customerIdx = console(customerText.format(customer_list=customers_str), len(customers))
    #if enter was pressed, exit from this function
    if customerIdx == "":
        return
    else:
        customerIdx -= 1
    accounts_str = ""
    #create str that will be output to the client console
    if accounts:
        for account in accounts:
            accounts_str += "\t\tAccount ID: " + str(account["account_id"]) + " Currency: " + account["currency"] + "\n"
    else:
        accounts_str = "None"
    accountID = console(accountText.format(account_list=accounts_str), len(accounts))
    #if enter was pressed, exit from this function
    if accountID == "":
        return
    requestAccount = {"call" : "getAccountByID", "account_id" : accountID}
    response = communicate("request", requestAccount)
    #exception handling
    if type(response) == dict and "response" in response.keys():
        response = response["response"]
    if response:
        requestConnection = {"call" : "postAccountConnection",
                             "account_id" : accountID,
                             "customer_id" : customers[customerIdx]["customer_id"]}
        resp = communicate("request", requestConnection)
        if "done" in resp:
            print("\t\tCreation successful! Press ENTER to go back")
            input()
        else:
            print("\t\tCreation failed! Press ENTER to go back")
            input()

def mainMenu():
    createSocket(True)
    menuText = """
        Hello Admin! Please select one of the following options:
        
        1. List all client application connections
        2. Kill connection to a certain client
        3. Create Customer account
        4. Connect an existing Customer to an existing Account
        5. Exit
    \t"""

    option = 0
    while option != 5:
        os.system(CLEAR_CMD)
        option = console(menuText, 5)
        if option == 1:
            listAll()
        elif option == 2:
            killSocket()
        elif option == 3:
            createCustomer()
        elif option == 4:
            connectCustomerToAccount()
    else:
        communicate("exit")
        exit()

mainMenu()