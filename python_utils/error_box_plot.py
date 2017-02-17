"""
Thanks Josh Hemann for the example
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc
import sys
import os
from matplotlib.patches import Polygon

def getRegError(path):
    print("On traite le fichier {}".format(path))
    file = open(path, 'r')
    i = 0
    R = np.ndarray(shape=(3,3))
    t = np.ndarray(shape=(3,1))
    for line in file:
        if i - 11 in range(0, 3):
            print(line)
            line = line.replace(']', '')
            line = line.replace('[', '')
            lineVector = line.split(' ')
            lineVector = [x for x in lineVector if x != '']
            R[i-11,0] = float(lineVector[0])
            R[i-11,1] = float(lineVector[1])
            R[i-11,2] = float(lineVector[2])
            t[i-11] = float(lineVector[3])
        i += 1
    return np.linalg.norm(R, ord=2), np.linalg.norm(t, ord=2)

rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})

# Generate some data from five different probability distributions,
# each with different characteristics. We want to play with how an IID
# bootstrap resample of the data preserves the distributional
# properties of the original sample, and a boxplot is one visual tool
# to make this assessment
numSites = 4
sites = [('Sequoia', 'resultSequoia'), 
         ('Valcartier 1', 'result1'),
         ('Valcartier 2', 'result5'),
         ('Savanne', 'result_savanne')]
siteNames = ['Sequoia',
             'Valcartier 1',
             'Valcartier 2',
             'Savanne']
directory = sys.argv[1]

dataTrans = []
dataRot = []
for iter in sites:
    filename = iter[1]
    site = iter[0]
    errorsRot = []
    errorsTrans = []
    for name in os.listdir(directory):
        if name.startswith(filename):
            path = os.path.join(directory, name)
            errorRot, errorTrans = getRegError(path)
            errorsRot.append(errorRot)
            errorsTrans.append(errorTrans)
    dataTrans.append(errorsTrans)
    dataRot.append(errorsRot)


fig, ax1 = plt.subplots(figsize=(10, 6))
fig.canvas.set_window_title('A Boxplot Example')
plt.subplots_adjust(left=0.075, right=0.95, top=0.9, bottom=0.25)

bp = plt.boxplot(dataTrans, notch=0, sym='+', vert=1, whis=1.5)
plt.setp(bp['boxes'], color='black')
plt.setp(bp['whiskers'], color='black')
plt.setp(bp['fliers'], color='red', marker='+')

# Add a horizontal grid to the plot, but make it very light in color
# so we can use it for reading data values but not be distracting
ax1.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',
               alpha=0.5)

# Hide these grid behind plot objects
ax1.set_axisbelow(True)
ax1.set_title('Translation error for each test site')
ax1.set_xlabel('Site')
ax1.set_ylabel(r'$||\mathbf{t}_{algorithm} - \mathbf{t}_{targets}||$ (meters)')

# Now fill the boxes with desired colors
boxColors = ['darkkhaki', 'royalblue']
numBoxes = numSites
medians = list(range(numBoxes))
for i in range(numBoxes):
    box = bp['boxes'][i]
    boxX = []
    boxY = []
    for j in range(5):
        boxX.append(box.get_xdata()[j])
        boxY.append(box.get_ydata()[j])
    boxCoords = list(zip(boxX, boxY))
    # Alternate between Dark Khaki and Royal Blue
    k = i % 2
    boxPolygon = Polygon(boxCoords, facecolor=boxColors[k])
    ax1.add_patch(boxPolygon)
    # Now draw the median lines back over what we just filled in
    med = bp['medians'][i]
    medianX = []
    medianY = []
    for j in range(2):
        medianX.append(med.get_xdata()[j])
        medianY.append(med.get_ydata()[j])
        plt.plot(medianX, medianY, 'k')
        medians[i] = medianY[0]
    # Finally, overplot the sample averages, with horizontal alignment
    # in the center of each box
    plt.plot([np.average(med.get_xdata())], [np.average(dataTrans[i])],
             color='w', marker='*', markeredgecolor='k')

# Set the axes ranges and axes labels
ax1.set_xlim(0.5, numBoxes + 0.5)
top = 40
bottom = -5
ax1.set_ylim(auto=True)
xtickNames = plt.setp(ax1, xticklabels=siteNames)
plt.setp(xtickNames, rotation=45, fontsize=8)

# Due to the Y-axis scale being different across samples, it can be
# hard to compare differences in medians across the samples. Add upper
# X-axis tick labels with the sample medians to aid in comparison
# (just use two decimal places of precision)
pos = np.arange(numBoxes) + 1
upperLabels = [str(np.round(s, 2)) for s in medians]
weights = ['bold', 'semibold']
for tick, label in zip(range(numBoxes), ax1.get_xticklabels()):
    k = tick % 2
    ax1.text(pos[tick], top - (top*0.05), upperLabels[tick],
             horizontalalignment='center', size='small', weight=weights[k],
             color=boxColors[k])

plt.show()
