import ioda._ioda_python as _iodapy
import copy
import datetime as dt
import numpy as np
import os

class ObsSpace:

    def __repr__(self):
        return f"ObsSpace({self.name},{self.iodafile})"

    def __str__(self):
        return f"IODA ObsSpace Object - {self.name}"

    def __init__(self, path, dim_dict=None, mode='r', name="NoName", iodalayout=0):
        self.name = name
        if mode not in ['r', 'w' ,'rw']:
            raise TypeError(f"{mode} not one of 'r','w','rw'")
        self.write = True if 'w' in mode else False
        self.read = True if 'r' in mode else False
        if  os.path.isfile(path) and self.read:
            self.iodafile = path
        elif os.path.exists(os.path.dirname(path)) and self.write:
            self.iodafile = path
        else:
            raise OSError(f"{path} does not specify a valid path to a file")
        self.iodalayout = iodalayout
        # read obsgroup from IODA
        if self.read:
            self.file, self.obsgroup = self._read_obsgroup(self.iodafile)
            self._def_classvars()
        else:
            if dim_dict is not None:
                self.file, self.obsgroup = self._create_obsgroup(self.iodafile, dim_dict)
            else:
                raise TypeError("dim_dict is not defined")

    def _def_classvars(self):
        # define some variables
        self.dimensions = self.obsgroup.vars.list()
        self.groups = self.file.listGroups(recurse=True)
        self.attrs = self.obsgroup.atts.list()
        self.nlocs = len(self.obsgroup.vars.open('nlocs').readVector.int())
        self.variables = self.file.listVars(recurse=True)
        self.nvars = len(self.variables)

    def write_attr(self, attrName, attrVal):
        # get type of variable
        try:
            attrType = self.NumpyToIodaDtype(attrVal)
            if attrType == _iodapy.Types.float:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.float(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.float(attrVal)
            elif attrType == _iodapy.Types.double:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.double(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.double(attrVal)
            elif attrType == _iodapy.Types.int64:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.int64(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.int64(attrVal)
            elif attrType == _iodapy.Types.int32:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.int32(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.int32(attrVal)
            # add other elif here TODO
        except AttributeError:  # if string
            if (type(attrVal) == str):
                attrType = _iodapy.Types.str
                self.obsgroup.atts.create(
                    attrName, attrType, [1]).writeDatum.str(attrVal)

    def _read_obsgroup(self, path):
        # open the obs group for a specified file and return file and obsgroup objects
        if self.write:
            iodamode = _iodapy.Engines.BackendOpenModes.Read_Write
        else:
            iodamode = _iodapy.Engines.BackendOpenModes.Read_Only
        g = _iodapy.Engines.HH.openFile(
                        name=path,
                        mode=iodamode,
                        )
        dlp = _iodapy.DLP.DataLayoutPolicy.generate(
                  _iodapy.DLP.DataLayoutPolicy.Policies(self.iodalayout))
        og = _iodapy.ObsGroup(g, dlp)
        return g, og

    def _create_obsgroup(self, path, dim_dict):
        # open a new file for writing and return file and obsgroup objects
        g = _iodapy.Engines.HH.createFile(
                        name=path,
                        mode=_iodapy.Engines.BackendCreateModes.Truncate_If_Exists,
                        )
        # create list of dims in obs group
        _dim_list = []
        for key, value in dim_dict.items():
            if key == 'nlocs':
                _nlocs_var = _iodapy.NewDimensionScale.int32(
                    'nlocs', value, _iodapy.Unlimited, value)
                _dim_list.append(_nlocs_var)
            else:
                _dim_list.append(_iodapy.NewDimensionScale.int32(
                key, value, value, value))
        # create obs group
        og = _iodapy.ObsGroup.generate(g, _dim_list)
        # default compression option
        self._p1 = _iodapy.VariableCreationParameters()
        self._p1.compressWithGZIP()
        return g, og

    def create_var(self, varname, groupname=None, dtype=np.dtype('float32'),
                   dim_list=['nlocs'], fillval=None):
        dtype_tmp = np.array([],dtype=dtype)
        typeVar = self.NumpyToIodaDtype(dtype_tmp)
        _varstr = varname
        if groupname is not None:
            _varstr = f"{groupname}/{varname}"
        # get list of dimension variables
        dims = [self.obsgroup.vars.open(dim) for dim in dim_list]
        fparams = copy.deepcopy(self._p1) # default values
        # replace default fill value
        if fillval is not None:
            fparams = setFillValue(fparams, typeVar, fillval)
        newVar = self.file.vars.create(_varstr, typeVar,
                                       scales=dims, params=fparams)

    def setFillValue(params, datatype, value):
        # set fill value for input VariableCreationParameters,
        # datatype, and value and return a new VariableCreationParameters
        # object with the new fill value
        if datatype == _iodapy.Types.float:
            params.setFillValue.float(value)
        elif datatype == _iodapy.Types.double:
            params.setFillValue.double(value)
        elif datatype == _iodapy.Types.int64:
            params.setFillValue.int64(value)
        elif datatype == _iodapy.Types.int32:
            params.setFillValue.int32(value)
        elif datatype == _iodapy.Types.str:
            params.setFillValue.str(value)
        # add other elif here TODO
        return params

    def Variable(self, varname, groupname=None):
        return self._Variable(self, varname, groupname)

    def NumpyToIodaDtype(self, NumpyArr):
        ############################################################
        # This method converts the numpy data type to the
        # corresponding ioda datatype

        NumpyDtype = NumpyArr.dtype

        if (NumpyDtype == np.dtype('float64')):
            IodaDtype = _iodapy.Types.double    # convert double to float
        elif (NumpyDtype == np.dtype('float32')):
            IodaDtype = _iodapy.Types.float
        elif (NumpyDtype == np.dtype('int64')):
            IodaDtype = _iodapy.Types.int64    # convert long to int
        elif (NumpyDtype == np.dtype('int32')):
            IodaDtype = _iodapy.Types.int32
        elif (NumpyDtype == np.dtype('int16')):
            IodaDtype = _iodapy.Types.int16
        elif (NumpyDtype == np.dtype('int8')):
            IodaDtype = _iodapy.Types.int16
        elif (NumpyDtype == np.dtype('S1')):
            IodaDtype = _iodapy.Types.str
        elif (NumpyDtype == np.dtype('object')):
            IodaDtype = _iodapy.Types.str
        else:
            try:
                a = str(NumpyArr[0])
                IodaDtype = _iodapy.Types.str
            except TypeError:
                print("ERROR: Unrecognized numpy data type: ", NumpyDtype)
                exit(-2)
        return IodaDtype

    class _Variable:
        def __repr__(self):
            return f"IODA variable ({self._varstr})"

        def __str__(self):
            return f"IODA variable object - {self._varstr}"

        def __init__(self, obsspace, varname, groupname=None):
            self.obsspace = obsspace
            self._varstr = varname
            if groupname is not None:
                self._varstr = f"{groupname}/{varname}"
            self._iodavar = self.obsspace.obsgroup.vars.open(self._varstr)
            self.attrs = self._iodavar.atts.list()

        def write_data(self, npArray):
            datatype = self.obsspace.NumpyToIodaDtype(npArray)
            if datatype == _iodapy.Types.float:
                self._iodavar.writeNPArray.float(npArray)
            elif datatype == _iodapy.Types.double:
                self._iodavar.writeNPArray.double(npArray)
            elif datatype == _iodapy.Types.int64:
                self._iodavar.writeNPArray.int64(npArray)
            elif datatype == _iodapy.Types.int32:
                self._iodavar.writeNPArray.int32(npArray)
            elif datatype == _iodapy.Types.str:
                self._iodavar.writeVector.str(npArray)
            # add other elif here TODO

        def write_attr(self, attrName, attrVal):
            # get type of variable
            try:
                attrType = self.obsspace.NumpyToIodaDtype(attrVal)
                if attrType == _iodapy.Types.float:
                    if len(attrVal) == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.float(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.float(attrVal)
                elif attrType == _iodapy.Types.double:
                    if len(attrVal) == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.double(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.double(attrVal)
                elif attrType == _iodapy.Types.int64:
                    if len(attrVal) == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.int64(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.int64(attrVal)
                elif attrType == _iodapy.Types.int32:
                    if len(attrVal) == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.int32(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.int32(attrVal)
                # add other elif here TODO
            except AttributeError:  # if string
                if (type(attrVal) == str):
                    attrType = _iodapy.Types.str
                    self._iodavar.atts.create(
                        attrName, attrType, [1]).writeDatum.str(attrVal)

        def read_data(self):
            """
              returns numpy array/python list of variable data
            """
            # figure out what datatype the variable is
            if self._iodavar.isA2(_iodapy.Types.float):
                # float
                data = self._iodavar.readNPArray.float()
                data[np.abs(data) > 9e36] = np.nan # undefined values
            elif self._iodavar.isA2(_iodapy.Types.int):
                # integer
                data = self._iodavar.readNPArray.int()
            elif self._iodavar.isA2(_iodapy.Types.str):
                # string
                data = self._iodavar.readVector.str() # ioda cannot read str to datetime...
                # convert to datetimes if applicable
                if "datetime" in self._varstr:
                    data = self._str_to_datetime(data)
            else:
                raise TypeError(f"Variable {self._varstr} type not supported")
            # reshape if it is a 2D array with second dim size 1
            try:
                if data.shape[1] == 1:
                    data = data.flatten()
            except (IndexError, AttributeError):
                pass
            return data

        def _str_to_datetime(self, datain):
            # comes as a list of strings, need to
            # make into datetime objects
            datetimes = [dt.datetime.strptime(x, "%Y-%m-%dT%H:%M:%SZ") for x in datain]
            return np.array(datetimes)
