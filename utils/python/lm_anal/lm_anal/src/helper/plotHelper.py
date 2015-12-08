__all__ = ['AxisIter', 'AxisList', 'HideAxesFrame']

def AxisIter(ax):
    for axis in (ax.get_xaxis(), ax.get_yaxis()):
        yield axis

def AxisList(ax):
    return [axis for axis in [ax.get_xaxis(), ax.get_yaxis()]]

def HideAxesFrame(ax):
    # ax.axis('off')

    ax.spines['top'].set_color('none')
    ax.spines['bottom'].set_color('none')
    ax.spines['left'].set_color('none')
    ax.spines['right'].set_color('none')

    ax.get_xaxis().set_ticks([])
    ax.get_yaxis().set_ticks([])

    # ax.get_xaxis().set_visible(False)
    # ax.get_yaxis().set_visible(False)

    for item in [ax]:
        item.patch.set_visible(False)

    ax.tick_params(top='off', bottom='off', left='off', right='off')    #labelcolor='w',