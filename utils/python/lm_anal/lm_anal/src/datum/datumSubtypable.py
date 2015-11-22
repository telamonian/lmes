from lm_anal.src.datum.datum import Datum, DatumMetaclass

class DatumSubtypableMetaclass(DatumMetaclass):
    '''
    we want all of our DatumSubtypables subclasses to have a cls.subtypeIDDict that's shared between all sub-, subsub-, etc classes
    '''
    def __new__(cls, clsname, bases, dct):
        # if a subtypeIDDict is not inherited (and this isn't DatumSubtypable), initialze subtypeIDDict
        if clsname!='DatumSubtypable':
            # sIDDFound = False
            # for base in bases:
            #     if hasattr(base, 'subtypeIDDict'):
            #         sIDDFound = True
            #         break
            # if not sIDDFound:
            #     dct['subtypeIDDict'] = {}
            clsObj = super().__new__(cls, clsname, bases, dct)
            if not hasattr(clsObj, 'subtypeIDDict'):
                clsObj.subtypeIDDict = {}
            if clsObj.subtypeID is not None:
                clsObj.subtypeIDDict[clsObj.subtypeID] = clsObj
            return clsObj
        else:
            return super().__new__(cls, clsname, bases, dct)

class DatumSubtypable(Datum, metaclass=DatumSubtypableMetaclass):
    subtypeID = None

    @classmethod
    def registerSubtype(cls, typeID):
        cls.subtypeIDDict[typeID] = cls

    def init(self):
        '''
        any code that needs to run when an object switches type should go here. should usually be called in __init__ as well in the "terminal" subtypes
        '''
        pass

    def setType(self, typeID):
        self.__class__ = self.subtypeIDDict[typeID]
        self.init()