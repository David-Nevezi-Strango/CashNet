import socket
import json
from copy import deepcopy
import os
import re

serverAddressPort = ("127.0.0.1", 5500)
bufferSize = 1024
TCPSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)
API_URL = 'https://openexchangerates.org/api/latest.json?app_id={api_key}&symbols={curr}&prettyprint=false&show_alternative=false'
CLEAR_CMD = "cls" if os.name == "nt" else "clear"
DIGIT_REGEX = re.compile('\d+((\.|,)\d+)?')

def createSocket(isAdmin=False):
    #function to connect to the server and identify type of client
    identifier = "admin" if isAdmin else "client"
    try:
        TCPSocket.connect(serverAddressPort)
    except Exception as e:
        print("\n\n\tThe Server is not running.\n\tTry again or contact us!\n\tError: {}\n\n".format(e))
        exit()
    communicate(identifier)

def console(text, range):
    #function to get an index of possible range
    trial = input(text)
    if trial == "":
        return trial
    elif trial != "" and DIGIT_REGEX.match(trial):
        option = int(trial)
    else:
        option = 0
    while(option < 1 or option > range):
        trial = input("\tInvalid number, please select from the options above!\n\t")
        #if enter was pressed, exit from this function
        if trial == "":
            return trial
        #exception handling
        elif trial != "" and DIGIT_REGEX.match(trial):
            option = int(trial)
    return option

def communicate(primitive: str, json_param=None):
    #function used for communication protocol
    global TCPSocket
    try:
        #if it is a identifying communication
        if primitive in ["client", "admin", "exit"]:
            TCPSocket.send(primitive.encode())
            TCPSocket.recv(bufferSize)
        else:
            #begin protocol
            json_str = json.dumps(json_param)
            TCPSocket.send(primitive.encode())
            respConfirmationPrimitive = TCPSocket.recv(bufferSize).decode()
            # print("1 ", respConfirmationPrimitive)
            TCPSocket.send(str(len(json_str)).encode())
            respConfirmationSize = TCPSocket.recv(bufferSize).decode()
            # print("2 ",respConfirmationSize)
            TCPSocket.send(json_str.encode())
            respConfirmationJson = TCPSocket.recv(bufferSize).decode()
            # print("3 ",respConfirmationJson)
            TCPSocket.send(json_str.encode())
            respSize = TCPSocket.recv(bufferSize).decode()
            # print("4 ",respSize)
            #if confirmation was sent instead of bytesize, return with confirmation
            if not DIGIT_REGEX.match(respSize):
                #exception handling
                if "{" in respSize:
                    return json.loads(respSize)
                else:
                    return respSize
            else:
                respSize = int(respSize)
            TCPSocket.send(str(respSize).encode())
            responseJson = ""
            #get response JSON
            while respSize > 0:
                responseJson += TCPSocket.recv(bufferSize).decode()
                size = len(responseJson)
                respSize -= size
                # print("5 ",responseJson)
                TCPSocket.send(str(respSize).encode())
            respConfirmation = TCPSocket.recv(bufferSize).decode()
            # print("6 ",respConfirmation)
            return json.loads(responseJson)
    except Exception as e:
        print("\n\n\tMost probably you got disconnected from the Server.\n\tTry again or contact us!\n\tError: {}\n\n".format(e))
        exit()

#SHA related functions
def SHA256_ROTR(x: int, n: int, w: int = 32):
    # rotate right
    return (x >> n) | (x << w - n)


def SHA256_SHR(x: int, n: int):
    return (x >> n)


def SHA256_Ch(x: int, y: int, z: int):
    # if x then y else z
    return (x & y) ^ (~x & z)


def SHA256_Maj(x: int, y: int, z: int):
    # majority of values
    return (x & y) ^ (x & z) ^ (y & z)


def SHA256_padding(message: bytearray) -> bytearray:
    length = len(message) * 8  # convert from bytes to bits
    message.append(0x80)  # add 1 bit
    while (len(message) * 8 + 64) % 512 != 0:  # pad until 448 bits (last 64 is reserved for len)
        message.append(0x00)
    message += length.to_bytes(8, 'big')  # pad to 8 bytes or 64 bits
    if ((len(message) * 8) % 512 == 0):
        return message
    else:
        print("\n\nError: Padding did not respect the 512 block constraint!\n\n")
        exit(0)


def SHA256_parse(message: bytearray) -> list:
    chunks = list()
    for i in range(0, len(message), 64):
        chunks.append(message[i:i + 64])
    return chunks


def SHA256_sigma1(chunk: bytearray):
    value = int.from_bytes(chunk, 'big')
    return SHA256_ROTR(value, 7) ^ SHA256_ROTR(value, 18) ^ SHA256_SHR(value, 3)


def SHA256_sigma0(chunk: bytearray):
    value = int.from_bytes(chunk, 'big')
    return SHA256_ROTR(value, 17) ^ SHA256_ROTR(value, 19) ^ SHA256_SHR(value, 10)


def SHA256_sum0(value: int):
    return SHA256_ROTR(value, 2) ^ SHA256_ROTR(value, 13) ^ SHA256_ROTR(value, 22)


def SHA256_sum1(value: int):
    return SHA256_ROTR(value, 6) ^ SHA256_ROTR(value, 11) ^ SHA256_ROTR(value, 25)


def sha_256(message: bytearray) -> int:
    # reference: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
    H_INITIAL = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]

    K = [0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
         0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
         0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
         0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
         0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
         0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
         0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
         0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2]

    # check if input is correct
    if isinstance(message, str):
        message = bytearray(message, 'ascii')
    elif isinstance(message, bytes):
        message = bytearray(message)
    elif not isinstance(message, bytearray):
        print("\n\nError: argument does not have the type of str, bytes or bytearray!\n\n")
        exit(0)
    # preprocess
    padded_message = SHA256_padding(message)
    parsed_message = SHA256_parse(padded_message)

    # init
    hash = deepcopy(H_INITIAL)

    # computation
    for chunk in parsed_message:
        schedule = list()
        for t in range(0, 64):
            if (t <= 15):
                # as defined in the standard, first, add 32 bits, t = 8 bits (each index represents 8 bits)
                schedule.append(bytes(chunk[t * 4:(t * 4) + 4]))
            else:
                s1 = SHA256_sigma1(schedule[t - 2])
                w1 = int.from_bytes(schedule[t - 7], 'big')
                s0 = SHA256_sigma0(schedule[t - 15])
                w2 = int.from_bytes(schedule[t - 16], 'big')
                # truncate result for 4 bytes
                result = ((s1 + w1 + s0 + w2) % 2 ** 32).to_bytes(4, 'big')
                schedule.append(result)

        if (len(schedule) != 64):
            print("\n\nError on Schedule, there are a different number of schedules than 64!\n\n")
            exit(0)

        a = hash[0]
        b = hash[1]
        c = hash[2]
        d = hash[3]
        e = hash[4]
        f = hash[5]
        g = hash[6]
        h = hash[7]

        for t in range(0, 64):
            t1 = (h + SHA256_sum1(e) + SHA256_Ch(e, f, g) + K[t] + int.from_bytes(schedule[t], 'big')) % 2 ** 32
            t2 = (SHA256_sum0(a) + SHA256_Maj(a, b, c)) % 2 ** 32
            h = g
            g = f
            f = e
            e = (d + t1) % 2 ** 32
            d = c
            c = b
            b = a
            a = (t1 + t2) % 2 ** 32

        # compute intermediate
        hash[0] = (hash[0] + a) % 2 ** 32
        hash[1] = (hash[1] + b) % 2 ** 32
        hash[2] = (hash[2] + c) % 2 ** 32
        hash[3] = (hash[3] + d) % 2 ** 32
        hash[4] = (hash[4] + e) % 2 ** 32
        hash[5] = (hash[5] + f) % 2 ** 32
        hash[6] = (hash[6] + g) % 2 ** 32
        hash[7] = (hash[7] + h) % 2 ** 32

    final = (hash[0].to_bytes(4, 'big') + hash[1].to_bytes(4, 'big') + hash[2].to_bytes(4, 'big') +
             hash[3].to_bytes(4, 'big') + hash[4].to_bytes(4, 'big') + hash[5].to_bytes(4, 'big') +
             hash[6].to_bytes(4, 'big') + hash[7].to_bytes(4, 'big'))

    return final.hex()