import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LogNorm

# modify this to decide which plots are done
plots = [\
        #  'times',\
        #  'comparison',\
        'filtered'\
]

def plot_intervals(data, col, title, cbar_label, output_filename, log_scale, min_value = None, max_value = None):
#if min_value is None:
#       min_value = data.min()
#   if max_value is None:
#       max_value = data.max()

    plt.rc('text', usetex=True)
    plt.rc('font', family='serif')
    plt.rc('font', size=11)

    intvlwidth=0.05

#column_labels = range(0, -11, -1) + range(-10, 1)
    column_labels = [ '$1-2^{' + str(x) + '}$' for x in range(-1,-11,-1)] + [ '$1+2^{' + str(x) + '}$' for x in range(-10,3, 1) ]
#row_labels = [ '$2^{' + str(x) + '}$' for x in range(1, 15, 2) ]

    fig, axis = plt.subplots()
    fig.suptitle(title + ':', fontsize=40, x=0.1, ha='left', va='baseline')
    fig.set_figwidth(20.)
    #  fig.set_figheight(10.)

#if log_scale:
#       heatmap = axis.pcolor(data, cmap='bone', norm=LogNorm(vmin=min_value, vmax=max_value))
#   else:
#       heatmap = axis.pcolor(data, cmap='hot_r', vmin=min_value, vmax=max_value)

    axis.set_ylabel(cbar_label, fontsize=30)
    axis.set_xlabel(r'distance factor', fontsize=30)
#    axis.set_yticks(np.arange(data.shape[0], step=2)+0.5, minor=False)
    axis.set_xticks(np.arange(data.shape[1], step=1), minor=False)

#    axis.invert_yaxis()

#    axis.set_yticklabels(row_labels, minor=False)
    axis.set_xticklabels(column_labels, minor=False)

    ind = np.arange(data.shape[1])
#axis.bar(ind, data[0], yerr=data[1]) 
    plt.axvline(9.5, color='k')
    plt.bar(ind-intvlwidth/2, data[col+2], width=intvlwidth, bottom=data[col+1], color='green')
    plt.plot(ind, data[col], 'og')
#    plt.errorbar(ind, data[col], fmt='.',yerr=data[col+1:col+3])
#cbar = plt.colorbar(heatmap)
#    cbar.set_label(cbar_label, fontsize=30)
#plt.show()
    plt.savefig(output_filename, bbox_inches='tight')

def plot_bbcalls(data, title, cbar_label, output_filename, log_scale, min_value = None, max_value = None):
    if min_value is None:
        min_value = data[3:9].min()
    if max_value is None:
        max_value = data[3:9].max()

    plt.rc('text', usetex=True)
    plt.rc('font', family='serif')
    plt.rc('font', size=11)

    intvlwidth=0.05

#column_labels = range(0, -11, -1) + range(-10, 1)
    column_labels = [ '$1-2^{' + str(x) + '}$' for x in range(-1,-11,-1)] + [ '$1+2^{' + str(x) + '}$' for x in range(-10,3, 1) ]
#row_labels = [ '$2^{' + str(x) + '}$' for x in range(1, 15, 2) ]

    fig, axis = plt.subplots()
    fig.suptitle(title + ':', fontsize=40, x=0.1, ha='left', va='baseline')
    fig.set_figwidth(20.)
    #  fig.set_figheight(10.)

#if log_scale:
#       heatmap = axis.pcolor(data, cmap='bone', norm=LogNorm(vmin=min_value, vmax=max_value))
#   else:
#       heatmap = axis.pcolor(data, cmap='hot_r', vmin=min_value, vmax=max_value)

    axis.set_ylabel(cbar_label, fontsize=30)
    axis.set_xlabel(r'distance factor', fontsize=30)
#    axis.set_yticks(np.arange(data.shape[0], step=2)+0.5, minor=False)
    axis.set_xticks(np.arange(data.shape[1], step=1), minor=False)

#    axis.invert_yaxis()

#    axis.set_yticklabels(row_labels, minor=False)
    axis.set_xticklabels(column_labels, minor=False)
    axis.set_yscale("log")


    ind = np.arange(data.shape[1])
#axis.bar(ind, data[0], yerr=data[1]) 
    plt.bar(ind-intvlwidth/2, data[8], width=intvlwidth, bottom=data[7], color='black')
    plt.plot(ind, data[6], 'ok')

    plt.axvline(9.5, color='k')
    plt.bar(ind-intvlwidth/2, data[5], width=intvlwidth, bottom=data[4], color='green')
    plt.plot(ind, data[5], 'og')

#    plt.errorbar(ind, data[col], fmt='.',yerr=data[col+1:col+3])
#cbar = plt.colorbar(heatmap)
#    cbar.set_label(cbar_label, fontsize=30)
#plt.show()
    plt.savefig(output_filename, bbox_inches='tight')



for dataset in [ 'same-characters','all-characters','sigspatial' ]:
    data = np.loadtxt(dataset + '.txt')
    cbar_label = 'Decision time (ms)'
    output_filename = dataset + '-times.pdf'
    plot_intervals(data.transpose(), 0, "\\textsc{" + dataset.capitalize() + "}", cbar_label, output_filename, False)
    cbar_label = 'Black-box calls'
    output_filename = dataset + '-bbcalls.pdf'
    plot_bbcalls(data.transpose(), "\\textsc{" + dataset.capitalize() + "}", cbar_label, output_filename, False)
