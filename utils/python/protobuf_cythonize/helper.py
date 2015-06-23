typeDict = {'::google::protobuf::int32': 'int32',
            '::google::protobuf::uint32': 'uint32'}

def ConvertType(toConv):
    if isinstance(toConv, basestring):
        return typeDict.get(toConv, toConv)
    else:
        return [typeDict.get(toC, toC) for toC in toConv]