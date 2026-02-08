#!/usr/bin/env python3
import re
import json
import sys


def read_file(path: str):
    if (path == '-'):
        return sys.stdin.buffer.read()
    else:
        with open(path, 'rb') as file:
            content = file.read()
        return content


def extract(text):
    # Initialize dictionary to store extracted data
    data = {}

    # Patterns for each field
    patterns = {
        'WorK_Key_Type': r'\*\*\s*(\S+)',
        'IK': r'IK:\s*([A-F0-9]+)',
        'KSN': r'KSN:\s*([A-F0-9]+)',
        'KSN_working': r'KSN \(working\):\s*([A-F0-9]+)',
        'Transaction_Counter': r'Transaction Counter:\s*([A-F0-9]+)',
        'Initial_Key_Id': r'Initial Key Id:\s*([A-F0-9]+)',
        'PIN_Encryption_Key': r'PIN Encryption Key:\s*([A-F0-9]+)',
        'Msg_Auth_Gen_Key': r'Msg\. Auth\. Gen\. Key:\s*([A-F0-9]+)',
        'Msg_Auth_Ver_Key': r'Msg\. Auth\. Ver\. Key:\s*([A-F0-9]+)',
        'M_Auth_Both_Ways_Key': r'M\. Auth\. Both Ways Key:\s*([A-F0-9]+)',
        'Data_Encr_Encrypt_Key': r'Data Encr\. Encrypt Key:\s*([A-F0-9]+)',
        'Data_Encr_Decrypt_Key': r'Data Encr\. Decrypt Key:\s*([A-F0-9]+)',
        'D_Encr_Both_Ways_Key': r'D\. Encr\. Both Ways Key:\s*([A-F0-9]+)',
        'Key_Encryption_Key': r'Key Encryption Key:\s*([A-F0-9]+)',
        'Key_Derivation_Key': r'Key Derivation Key:\s*([A-F0-9]+)'
    }

    # Extract data using patterns
    for key, pattern in patterns.items():
        match = re.search(pattern, text)
        if match:
            data[key] = match.group(1)


    return data


def main():
    input = read_file(sys.argv[1]) if len(sys.argv) > 1 else read_file('-')
    text = input.decode(encoding='utf-8')

    datas = []
    for part in text.split('==' * 4):
        extracted_data = extract(part)
        if extracted_data == {}:
            continue
        datas.append(extracted_data)
    print(json.dumps(datas, indent=2))


if __name__ == "__main__":
    main()
