import json

data = json.loads(open('version_info.json').read())

def get_ecb_inner(ecb):
    return f"{{{ecb['count']}, {ecb['dataCodewords']}}}"

def get_ecb(ecb_group):
    return f"{{{ecb_group['ecCodewordsPerBlock']}, {get_ecb_inner(ecb_group['ecBlocks'][0])}" + \
           ('}' if len(ecb_group['ecBlocks']) == 1 else ', ' + get_ecb_inner(ecb_group['ecBlocks'][1]) + "}")

def get_version(version, version_data):
    alignment = version_data['alignment']
    ecBlocks = version_data['ecBlocks']
    totalCodewords = 0
    ecb_l = ecBlocks[0]
    assert len(ecBlocks) == 4
    for x in ecb_l['ecBlocks']:
        totalCodewords += x['count'] * (x['dataCodewords'] + ecb_l['ecCodewordsPerBlock'])
    return f'{{{version}, {len(alignment)}, {{{(", ".join(map(str, alignment)))}}}, ' \
           f'{totalCodewords}, {", ".join(map(get_ecb, ecBlocks))} }}'

def get_versions(data):
    res = []
    for i, x in enumerate(data):
        version = i + 1
        res.append(get_version(version, x))
    return '{ ' + ',\n'.join(res) + " }"

print(get_versions(data))