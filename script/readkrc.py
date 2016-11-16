import sys, zlib

def read_krc(filename):
    content = open(filename, 'rb').read()
    keycode = [64, 71, 97, 119, 94, 50, 116, 71, 81, 54, 49, 45, 206, 210, 110, 105]
    keysize = len(keycode)
    outtext = []
    index = 0
    for ch in content[4:]:
        cc = ord(ch) ^ keycode[index]
        index += 1
        if index >= keysize:
            index = 0
        outtext.append(chr(cc))
    content = ''.join(outtext)
    content = zlib.decompress(content)
    return content.decode('utf-8', 'ignore')


if __name__ == '__main__':
    text = read_krc('translate.krc')
    print text.encode('gbk', 'ignore')




