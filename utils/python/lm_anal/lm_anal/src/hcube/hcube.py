class HCube(object):
    def _call(self, *args, **kwargs):
        newDict = MagicDict()
        for oldKey,oldVal in self.map.items():
            if oldVal is None:
                newDict[oldKey] = None
            else:
                try:
                    newDict[oldKey] = oldVal.__call__(*args, **kwargs)
                except:
                    newDict[oldKey] = None
        newSims = self.getShallowCopy()
        newSims.map = newDict
        return newSims

    def __call__(self, *args, **kwargs):
        archetype = next(self.map.values().__iter__())
        if hasattr(archetype, '__name__') and archetype.__name__[:4]=='plot':
            return self._plotGrid(self, *args, **kwargs)
        else:
            return self._call(*args, **kwargs)

    def __delitem__(self, key):
        del self.map[key]

    def __getitem__(self, key):
        val = self.map[key]
        if isinstance(val, MagicDict):
            simsView = self.getShallowCopy()
            simsView.map = val
            return simsView
        else:
            return val

    def __getattr__(self, name):
        # this gets called if an attr isn't found in this Sims object
        if name=='__setstate__':    # or name=='__copy__' or name=='__reduce_ex__':
            # this is __getattr__ speak for 'you're out of luck, buddy'
            raise AttributeError
        if self.copying:
            raise AttributeError
            #return object.__getattr__(self, name)
        newDict = MagicDict()
        for oldKey,oldVal in self.map.items():
            if oldVal is None:
                newDict[oldKey] = None
            else:
                try:
                    newDict[oldKey] = oldVal.__getattribute__(name)
                except:
                    newDict[oldKey] = None
        newSims = self.getShallowCopy()
        newSims.map = newDict
        return newSims

    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.__iter__()

    def _addGridGlobalLabels(self, fig, grid, xLabel=None, yLabel=None, suptitle=None):
        fontScaler = 2.0/3.0
        fontSizeGlobalLabelX,fontSizeGlobalLabelY = np.array(self._getFontSizesFromFigSize(fig))*fontScaler

        ax = fig.add_subplot(111, zorder=-1000)
        hlp.HideAxesFrame(ax)

        fontSizeAxLabelX,fontSizeAxLabelY = self._getFirstGoodGridObj(grid).getFontSizesFromAxesSize()

        ax.set_xlabel(xLabel, labelpad=fontSizeAxLabelX, size=fontSizeGlobalLabelX)
        ax.set_ylabel(yLabel, labelpad=fontSizeAxLabelY*3, size=fontSizeGlobalLabelY)

        suptitle = str(suptitle)
        figSize = self._getFigSizeInPoints(fig)
        fontSizeGlobalTitle = float(figSize[0])/len(suptitle)
        # 1 plus a multiple of fontSizeAxLabelX in units of fig height
        yPosTitle = 1 + (float(fontSizeAxLabelX)/figSize[1])*2
        ax.set_title(suptitle, y=yPosTitle, size=fontSizeGlobalTitle, zorder=10)

        # xPad = self._getFigPadFracFromFontSize(dim=0, fig=fig, fontSize=fontSizeGlobalLabelX)
        # yPad = self._getFigPadFracFromFontSize(dim=1, fig=fig, fontSize=fontSizeGlobalLabelY)

        # plt.annotate(xLabel, xy=(0.5, 0), xytext=(0, 0),
        #                     xycoords='figure fraction', textcoords='offset points',
        #                     size=fontSizeGlobalLabelX, ha='center', va='top')

        # xKwargs = {'x':0.5, 'y':-xPad, 'text':xLabel, 'ha':'center', 'size':fontSizeGlobalLabelX, 's':None}
        # yKwargs = {'x':-yPad, 'y':0.5, 'text':yLabel, 'va':'center', 'rotation':'vertical', 'size':fontSizeGlobalLabelY, 's':None}
        # for kwargs in [kwargs for kwargs in [xKwargs, yKwargs] if kwargs['text'] is not None]:
        #     fig.text(**kwargs)
        return fontSizeGlobalLabelX,fontSizeGlobalLabelY

    def _addGridColumnLabels(self, axArr, axArrMask, grid, stickyLabels=True):
        '''
        if stickyLabels, the column label "sticks" to the topmost good plot in the column
        otherwise, the column labels will always hug the subplots area's top margin
        '''
        dim = 0

        # if there's only one column, skip this
        if grid.shape[0] < 2:
            return

        pad = 5 # in points
        columnLabels = self._genGridColumnLabels(grid)

        if stickyLabels:
            axIter = self._iterFirstGoodAx(axArr=axArr, axArrMask=axArrMask, dim=~dim)
        else:
            axIter = axArr[0,:].ravel()

        for ax,label in zip(axIter, columnLabels):
            ax.annotate(label, xy=(0.5, 1), xytext=(0, pad),
                        xycoords='axes fraction', textcoords='offset points',
                        size='large', ha='center', va='baseline')

    def _addGridRowLabels(self, axArr, axArrMask, grid, stickyLabels=True):
        '''
        if stickyLabels, the row label "sticks" to the rightmost good plot in the row
        otherwise, the row labels will always hug the subplots area's right margin
        '''
        dim = 1

        # if there's only one row, skip this
        if grid.shape[dim] < 2:
            return

        pad = 5 # in points
        rowLabels = self._genGridRowLabels(grid)

        if stickyLabels:
            axIter = self._iterFirstGoodAx(axArr=axArr, axArrMask=axArrMask, dim=~dim, reverse=dim)
        else:
            axIter = axArr[:,-1].ravel()

        for ax,label in zip(axIter, rowLabels):
            ax.annotate(label, xy=(1, 0.5), xytext=(pad, 0),
                        xycoords='axes fraction', textcoords='offset points',
                        size='large', ha='left', va='center', rotation=270)     #'vertical')

    def _formatAxLabels(self, axArr, axArrMask, grid, stickyTicklabels):
        # turn off unnecessary axis and ticklabels
        for ax in axArr.ravel():
            # this won't work when looping over the axis' for some reason, so be explicit
            ax.set_xlabel('')
            ax.set_ylabel('')
            for i,axis in enumerate((ax.get_xaxis(), ax.get_yaxis())):
                for tickText in axis.get_majorticklabels():
                    tickText.set_visible(False)
                    # this is going to need to be more complex to do the tick pruning I wanted...
                    # for tickText in ax.get_xaxis().get_majorticklabels()[0:1] + ax.get_xaxis().get_majorticklabels()[-1:]:
                    #     print(tickText.get_visible())
                    #     tickText.set_visible(False)

        # # turn back on the necessary ticklabels
        # for dim in (0,1):
        #     for axVector,axMaskVector in zip(np.rollaxis(axArr, dim), np.rollaxis(axArrMask, dim)):
        #         # deal with the whole "Y-origin on top" thing via stride
        #         stride = -1 if dim==1 else 1
        #         for ax,axMask in zip(axVector.ravel()[::stride], axMaskVector.ravel()[::stride]):
        #             if not axMask:
        #                 axis = hlp.AxisList(ax)[~dim]
        #                 for tickText in axis.get_majorticklabels():
        #                     tickText.set_visible(True)
        #                 break
        for dim in (0,1):
            # deal with the whole "Y-origin on top" thing via reverse
            reverse = True if dim==1 else False
            axIter = self._iterFirstGoodAx(axArr=axArr, axArrMask=axArrMask, dim=dim, reverse=reverse)
            if stickyTicklabels:
                for ax in axIter:
                    axis = hlp.AxisList(ax)[~dim]
                    for tickText in axis.get_majorticklabels():
                        tickText.set_visible(True)
            else:
                axIterVisible = self._iterFirstAx(axArr=axArr, dim=dim, reverse=reverse)
                for ax,axVisible in zip(axIter, axIterVisible):
                    axis = hlp.AxisList(ax)[~dim]
                    axisVisible = hlp.AxisList(axVisible)[~dim]

                    if dim==0:
                        ylim = ax.get_ylim()
                    elif dim==1:
                        xlim = ax.get_xlim()

                    axisVisible.set_ticks(axis.get_ticklocs())
                    for tickText in axisVisible.get_majorticklabels():
                        tickText.set_visible(True)

                    if dim==0:
                        axVisible.set_ylim(ylim)
                    elif dim==1:
                        axVisible.set_xlim(xlim)

                    obj = self._getFirstGoodGridObj(grid)
                    obj.resizeTickLabels(ax=axVisible)

    def _formatGridPlot(self, fig, axArr, grid, axArrMask, singletonElems, stickyLabels, stickyTicklabels):
        # the grid cotains bound functions instead of datums, so we need the below ugliness to get at datum.getXLabel()
        archetype = self._getFirstGoodGridObj(grid)
        figXLabel = archetype.getXLabel()
        figYLabel = archetype.getYLabel()

        figSuptitle = singletonElems

        self._formatAxLabels(axArr=axArr, axArrMask=axArrMask, grid=grid, stickyTicklabels=stickyTicklabels)

        # for now, set every axes to have an equal aspect ratio. may want to add way to turn this on/off at the datum level
        # if not ax.get_xscale()=='log' and not ax.get_yscale()=='log':
        #     ax.set_aspect('equal')

        self._addGridRowLabels(axArr=axArr, axArrMask=axArrMask, grid=grid, stickyLabels=stickyLabels)
        self._addGridColumnLabels(axArr=axArr, axArrMask=axArrMask, grid=grid, stickyLabels=stickyLabels)

        self._addGridGlobalLabels(fig=fig, grid=grid, xLabel=figXLabel, yLabel=figYLabel, suptitle=figSuptitle)
        fig.tight_layout()

    def _genGridColumnLabels(self, grid):
        return self._genGridLineLabels(grid, dim=0)

    def _genGridRowLabels(self, grid):
        return self._genGridLineLabels(grid, dim=1)

    def _genGridLineLabels(self, grid, dim):
        labels = []
        lineSlice = self._getLineSlice(arr=grid, dim=dim)
        for i,gridLabel in enumerate(grid['label'][lineSlice].ravel()):
            # difference out the label that changes along this dim
            otherSet = set([tup for tups in grid['label'][lineSlice][:i] for tup in tups] +
                           [tup for tups in grid['label'][lineSlice][i+1:] for tup in tups])
            # this set should contain only a single tuple
            uniqueLabelSet = set(gridLabel) - otherSet
            # stupid iterator crap to get the 'first' element in a set
            labels.append(next(uniqueLabelSet.__iter__()))
        return labels

    def _getFigPadFracFromFontSize(self, dim, fig, fontSize):
        extent = fig.get_size_inches()[dim]
        return hlp.PointsToInches(fontSize)/extent

    def _getFigSizeInPoints(self, fig):
        figSize = []
        for length in fig.get_size_inches():
            figSize.append(hlp.InchesToPoints(length))
        return figSize

    def _getFirstGoodGridObj(self, grid):
        for obj in grid['obj'].ravel():
            if obj is not None:
                if obj.__self__ is not None:
                    return obj.__self__

    def _getFontSizesFromFigSize(self, fig, scaleFactor=.07):
        fontSizes = []
        extent = min(fig.get_size_inches())
        for length in fig.get_size_inches():
            fontSizeInInches = extent*float(scaleFactor)
            fontSizes.append(hlp.InchesToPoints(fontSizeInInches))
        return fontSizes

    def _getLineSlice(self, arr, dim, fixedIndex=0):
        '''
        return a 1D slice taken along the dim axis, with the values on the other axes held fixed
        eg if len(arr.shape)==4 and dim==2, lineSlice = arr[0,0,:,0]
        '''
        return (fixedIndex,)*dim + (slice(None),) + (fixedIndex,)*(len(arr.shape) - dim - 1)

    def _iterFirstAx(self, axArr, dim, reverse=False):
        fixedIndex = -1 if reverse else 0
        lineSlice = self._getLineSlice(arr=axArr, dim=dim, fixedIndex=fixedIndex)
        return axArr[lineSlice]

    def _iterFirstGoodAx(self, axArr, axArrMask, dim, reverse=False):
        for axVector,axMaskVector in zip(np.rollaxis(axArr, dim), np.rollaxis(axArrMask, dim)):
            stride = -1 if reverse else 1
            for ax,axMask in zip(axVector.ravel()[::stride], axMaskVector.ravel()[::stride]):
                if not axMask:
                    yield ax
                    break

    def _plotGrid(self, *args, **kwargs):
        # self ends up at the front of args (due to the nature of method signatures), so pop it off
        args = list(args); args.pop(0)
        grid,singletonElems = self.getGrid()

        cbar = kwargs.pop('cbar') if 'cbar' in kwargs else False
        cbarKwargs = kwargs.pop('cbarKwargs') if 'cbarKwargs' in kwargs else {}

        stickyLabels = kwargs.pop('stickyLabels') if 'stickyLabels' in kwargs else True
        stickyTicklabels = kwargs.pop('stickyTicklabels') if 'stickyTicklabels' in kwargs else True

        # unroll higher-D grids into 2D grids
        axArrShape = (np.product(grid.shape[1::2], dtype=int), np.product(grid.shape[::2], dtype=int))
        fig,axArr = self._setupFig(axArrShape, kwargs)
        # .subplots() flattens away dimensions of length 1, but we want a 2D axArr so add them back in if necessary
        if not isinstance(axArr, np.ndarray):
            axArr = np.array([axArr], dtype='O').reshape(1,1)
        elif len(axArr.shape) < 2:
            axArr = axArr.reshape(1,-1)

        # delay turning any axes off until after formatting to preserve correct placement/spacing
        badAxes = []
        axArrMask = np.zeros(grid.shape, dtype=bool)
        for i,(datumPlotFunc,ax) in enumerate(zip(grid['obj'].ravel(), axArr.T.ravel())):
            if datumPlotFunc is not None:
                datumPlotFunc.__call__(fig=fig, ax=ax, *args, **kwargs)
            else:
                badAxes.append(i)
                axArrMask.ravel()[i] = True
                # fig.delaxes(ax)
                # ax.set_frame_on(False)
        axArrMask = axArrMask.T
        if cbar:
            self._plotGridCbar(fig=fig, grid=grid, cbarKwargs=cbarKwargs)
        for ax in axArr[axArrMask].ravel():
            ax.axis('off')
            # for ax in (axArr.ravel()[i] for i in badAxes):
            # ax.axis('off')
            # hlp.HideAxesFrame(ax)
        self._formatGridPlot(fig=fig, axArr=axArr, grid=grid, axArrMask=axArrMask, singletonElems=singletonElems, stickyLabels=stickyLabels, stickyTicklabels=stickyTicklabels)
        return fig, axArr, grid

    def _plotGridCbar(self, fig, grid, cbarKwargs):
        archetype = self._getFirstGoodGridObj(grid)
        archetype.plotColorbar(**cbarKwargs)

    def _setupFig(self, axArrShape, kwargs):
        if 'fig' in kwargs and 'axArr' in kwargs:
            fig = kwargs.pop('fig')
            axArr = kwargs.pop('axArr')
        else:
            figKwargs = kwargs.pop('figKwargs') if 'figKwargs' in kwargs else {}
            subplot_kw = kwargs.pop('axesKwargs') if 'axesKwargs' in kwargs else {}
            sharex = kwargs.pop('sharex') if 'sharex' in kwargs else False
            sharey = kwargs.pop('sharey') if 'sharey' in kwargs else False
            if 'extent' in kwargs:
                figKwargs['figsize'] = np.array(axArrShape)[::-1]*kwargs.pop('extent')
            elif 'figsize' not in figKwargs:
                figKwargs['figsize'] = np.array(axArrShape)[::-1]*8
            fig, axArr = plt.subplots(*axArrShape, gridspec_kw={}, sharex=sharex, sharey=sharey, subplot_kw=subplot_kw, **figKwargs)
        return fig,axArr

    def getGrid(self):
        return self.map.getGrid()

    def getGridShape(self):
        return self.map.getGrid()[0].shape

    def getShallowCopy(self):
        self.copying = True
        retCopy = shallowCopy(self)
        self.copying = False
        retCopy.copying = False
        return retCopy

    # explicit functional methods for dealing with objects in the underlying MagicDict
    def call(self, *args, **kwargs):
        return self._call(*args, **kwargs)

    def get(self, name):
        return self.__getattr__(name)

    def getItem(self, name):
        return self.get('__getitem__')(name)

    # explicit functional methods for "collapsing" the Sims object and returning simple iterators/lists, or in some cases values
    def getItems(self):
        return list(self.items())

    def getKeys(self):
        return list(self.keys())

    def getValues(self):
        return list(self.values())

    def items(self):
        return self.map.items()

    def keys(self):
        return self.map.keys()

    def peek(self):
        return self.map.peek()

    def values(self):
        return self.map.values()

    # spark RDD-like methods
    def mapFunc(self, func, *args, **kwargs):
        doRaise = kwargs.pop('doRaise') if 'doRaise' in kwargs else False

        newDict = MagicDict()
        for oldKey,oldVal in self.map.items():
            if oldVal is None:
                newDict[oldKey] = None
            else:
                try:
                    newDict[oldKey] = func(oldVal, *args, **kwargs)
                except Exception as e:
                    if doRaise:
                        raise e
                    else:
                        newDict[oldKey] = None
        newSims = self.getShallowCopy()
        newSims.map = newDict
        return newSims