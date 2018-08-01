import os
import json
import chardet

count=0
items=[x for x in os.listdir('.') if os.path.isfile(x)]

for i in items:
    with open(i, 'rb') as data:
        encode=chardet.detect(data.read())
    with open(i, 'r',encoding=encode.get('encoding')) as data:
        try:
            parsed=json.load(data)
            count+=1
            print(parsed)
        except:
            print(i+" is skipped.")
print(count)
