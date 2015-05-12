from numpy import arange,around,array,asarray,atleast_1d,atleast_2d,bincount,diff,digitize,empty,isscalar,log10,ones,sort,where,zeros
import numpy as np
import re

def CamelCase(s):
    '''
    convert attrName to camelCase
    '''
    output = s
    for i,l in enumerate(output):
        if l.isupper():
            output = output[:i] + l.lower() + output[i+1:]
        else:
            return output
    return output

def ShortenName(s):
    '''
    input: CamelCase style name 
    output: abbreviated form (in lowercase) generated from the first letter of each "word"
    '''
    return ''.join(re.findall('[A-Z]', s)).lower()

def Singular(s):
    '''
    returns the singular form of a plural name
    '''
    if s[-3:]=='ies':
        return s[:-3] + 'y'
    elif s[-1]=='s':
        return s[:-1]

def histogramdd(sample, bins=10, range=None, normed=False, weights=None, includeOutliers=True):
    """
    my special version of the numpy function, with the includeOutliers keyword
    Compute the multidimensional histogram of some data.
    Parameters
    ----------
    sample : array_like
        The data to be histogrammed. It must be an (N,D) array or data
        that can be converted to such. The rows of the resulting array
        are the coordinates of points in a D dimensional polytope.
    bins : sequence or int, optional
        The bin specification:
        * A sequence of arrays describing the bin edges along each dimension.
        * The number of bins for each dimension (nx, ny, ... =bins)
        * The number of bins for all dimensions (nx=ny=...=bins).
    range : sequence, optional
        A sequence of lower and upper bin edges to be used if the edges are
        not given explicitly in `bins`. Defaults to the minimum and maximum
        values along each dimension.
    normed : bool, optional
        If False, returns the number of samples in each bin. If True,
        returns the bin density ``bin_count / sample_count / bin_volume``.
    weights : array_like (N,), optional
        An array of values `w_i` weighing each sample `(x_i, y_i, z_i, ...)`.
        Weights are normalized to 1 if normed is True. If normed is False,
        the values of the returned histogram are equal to the sum of the
        weights belonging to the samples falling into each bin.
    Returns
    -------
    H : ndarray
        The multidimensional histogram of sample x. See normed and weights
        for the different possible semantics.
    edges : list
        A list of D arrays describing the bin edges for each dimension.
    See Also
    --------
    histogram: 1-D histogram
    histogram2d: 2-D histogram
    Examples
    --------
    >>> r = np.random.randn(100,3)
    >>> H, edges = np.histogramdd(r, bins = (5, 8, 4))
    >>> H.shape, edges[0].size, edges[1].size, edges[2].size
    ((5, 8, 4), 6, 9, 5)
    """

    try:
        # Sample is an ND-array.
        N, D = sample.shape
    except (AttributeError, ValueError):
        # Sample is a sequence of 1D arrays.
        sample = atleast_2d(sample).T
        N, D = sample.shape

    nbin = empty(D, int)
    edges = D*[None]
    dedges = D*[None]
    if weights is not None:
        weights = asarray(weights)

    try:
        M = len(bins)
        if M != D:
            raise AttributeError(
                'The dimension of bins must be equal to the dimension of the '
                ' sample x.')
    except TypeError:
        # bins is an integer
        bins = D*[bins]

    # Select range for each dimension
    # Used only if number of bins is given.
    if range is None:
        # Handle empty input. Range can't be determined in that case, use 0-1.
        if N == 0:
            smin = zeros(D)
            smax = ones(D)
        else:
            smin = atleast_1d(array(sample.min(0), float))
            smax = atleast_1d(array(sample.max(0), float))
    else:
        smin = zeros(D)
        smax = zeros(D)
        for i in arange(D):
            smin[i], smax[i] = range[i]

    # Make sure the bins have a finite width.
    for i in arange(len(smin)):
        if smin[i] == smax[i]:
            smin[i] = smin[i] - .5
            smax[i] = smax[i] + .5

    # avoid rounding issues for comparisons when dealing with inexact types
    if np.issubdtype(sample.dtype, np.inexact):
        edge_dt = sample.dtype
    else:
        edge_dt = float
    # Create edge arrays
    for i in arange(D):
        if isscalar(bins[i]):
            if bins[i] < 1:
                raise ValueError(
                    "Element at index %s in `bins` should be a positive "
                    "integer." % i)
            nbin[i] = bins[i] + 2  # +2 for outlier bins
            edges[i] = linspace(smin[i], smax[i], nbin[i]-1, dtype=edge_dt)
        else:
            edges[i] = asarray(bins[i], edge_dt)
            nbin[i] = len(edges[i]) + 1  # +1 for outlier bins
        dedges[i] = diff(edges[i])
        if np.any(np.asarray(dedges[i]) <= 0):
            raise ValueError(
                "Found bin edge of size <= 0. Did you specify `bins` with"
                "non-monotonic sequence?")

    nbin = asarray(nbin)

    # Handle empty input.
    if N == 0:
        return np.zeros(nbin-2), edges

    # Compute the bin number each sample falls into.
    Ncount = {}
    for i in arange(D):
        Ncount[i] = digitize(sample[:, i], edges[i])

    # Using digitize, values that fall on an edge are put in the right bin.
    # For the rightmost bin, we want values equal to the right edge to be
    # counted in the last bin, and not as an outlier.
    for i in arange(D):
        # Rounding precision
        mindiff = dedges[i].min()
        if not np.isinf(mindiff):
            decimal = int(-log10(mindiff)) + 6
            # Find which points are on the rightmost edge.
            not_smaller_than_edge = (sample[:, i] >= edges[i][-1])
            on_edge = (around(sample[:, i], decimal) ==
                       around(edges[i][-1], decimal))
            # Shift these points one bin to the left.
            Ncount[i][where(on_edge & not_smaller_than_edge)[0]] -= 1

    # Flattened histogram matrix (1D)
    # Reshape is used so that overlarge arrays
    # will raise an error.
    hist = zeros(nbin, float).reshape(-1)

    # Compute the sample indices in the flattened histogram matrix.
    ni = nbin.argsort()
    xy = zeros(N, int)
    for i in arange(0, D-1):
        xy += Ncount[ni[i]] * nbin[ni[i+1:]].prod()
    xy += Ncount[ni[-1]]

    # Compute the number of repetitions in xy and assign it to the
    # flattened histmat.
    if len(xy) == 0:
        return zeros(nbin-2, int), edges

    flatcount = bincount(xy, weights)
    a = arange(len(flatcount))
    hist[a] = flatcount

    # Shape into a proper matrix
    hist = hist.reshape(sort(nbin))
    for i in arange(nbin.size):
        j = ni.argsort()[i]
        hist = hist.swapaxes(i, j)
        ni[i], ni[j] = ni[j], ni[i]

    # Remove outliers (indices 0 and -1 for each dimension).
    if not includeOutliers:
        core = D*[slice(1, -1)]
        hist = hist[core]

    # Normalize if normed is True
    if normed:
        s = hist.sum()
        for i in arange(D):
            shape = ones(D, int)
            shape[i] = nbin[i] - 2
            hist = hist / dedges[i].reshape(shape)
        hist /= s

    if includeOutliers:
        if (hist.shape != nbin).any():
            raise RuntimeError(
                "Internal Shape Error")
    else:
        if (hist.shape != nbin - 2).any():
            raise RuntimeError(
                "Internal Shape Error")
    return hist, edges