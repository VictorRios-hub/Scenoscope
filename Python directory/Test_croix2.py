import xlrd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
from tkinter import *
import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import time
import xlwt


#Recup Capteurs 1 ligne
Capteurs = []
#Recup orientation
OrientationData = []
Orientation = []
#Apres trilateration etablissement point
image = []
pts=[]
feuille_2=[]
feuille_1=[]
Orientation=[]
Figscatter=[]
ax=[]
init_angle=-90
workbook=[]
sheet=[]
sw_record=0
cpoint=0
Hsize=400
Wsize=500

   
def close_window (): 
    root.destroy()

#annotation (inclinaison
def add_annotation(annotated_message,x,y):
    global ax
    annotation = ax.annotate(annotated_message, 
                 xy = (x, y), xytext = (10, 10),
                 textcoords = 'offset points', ha = 'right', va = 'bottom',
                 bbox = dict(boxstyle = 'round,pad=0.1', fc = 'yellow', alpha = 0.5),
                 arrowprops = dict(arrowstyle = '->', connectionstyle = 'arc3,rad=0'))
    return annotation    
   
def openExpoFiles():
    global image, cols, rows, feuille_1, feuille_2, ax, Figscatter
    ######### image
    image = mpimg.imread("C:/Users/polytech/Documents/0Maelynn/scenoscope/IHM Python/plan_expo.png")
    image=np.flipud(image)
    #########
    
    ######### excel fichier acquisition
    document = xlrd.open_workbook("C:/Users/polytech/Documents/0Maelynn/scenoscope/IHM Python/DATALOG.xls")
    feuille_1 = document.sheet_by_index(0)
    #Nb colonnes + lignes
    cols = feuille_1.ncols
    rows = feuille_1.nrows
    #########
    
    ######### excel fichier emplacement tags RFID
    document2 = xlrd.open_workbook("C:/Users/polytech/Documents/0Maelynn/scenoscope/IHM Python/Tags.xls")
    feuille_2 = document2.sheet_by_index(0)
    
    #############   
    figure = plt.Figure(figsize=(4,5), dpi=200)
    ax = figure.add_subplot(111)
    ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
    Figscatter = FigureCanvasTkAgg(figure, root) 
    Figscatter.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH)
    
def plotPosition(index,sw_ann):
    global ax, Figscatter, init_angle
    ax.clear()       
    t = mpl.markers.MarkerStyle(marker=6)
    t._transform = t.get_transform().rotate_deg(Orientation[index]+init_angle)
    x = int(float(pts[index,0]))
    y = int(float(pts[index,1]))        
    ax.scatter(x, y, marker=t, color="red", s=70, alpha=0.5)
    ax.scatter(x, y, marker=".", color="black", s=50)    #marker=forme, s=epaisseur  
    if sw_ann==1:
        ann = add_annotation(pts[index,2], x, y)
    ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
    Figscatter.draw()

def record():
    global workbook, sheet, sw_record, cpoint
    cpoint=0
    workbook = xlwt.Workbook()
    sheet = workbook.add_sheet('feuille1')   
    sw_record=1
    
def onclick(event):
    global sw_record, sheet, cpoint, ax, Figscatter,workbook
    print("Single Click, Mouse position: (%s %s)" % (event.x, event.y))

    inv = ax.transData.inverted()
    val=inv.transform((event.x,  event.y))
    print(val)
    x=int(val[0])
    y=int(406-val[1])
    print(x," ",y)
    print(sw_record)

    if sw_record==1:
        sheet.write(cpoint, 0, cpoint)
        sheet.write(cpoint, 1, x)
        sheet.write(cpoint, 2, y)
        workbook.save("C:/Users/polytech/Documents/0Maelynn/scenoscope/IHM Python/TAG_DATA.xls")
        #EPC=input("Give the EPC of the new tag")
        second = tk.Tk()
        tk.Label(second, text="EPC:").grid(row=0)
        epc = tk.Entry(second)
        epc.grid(row=0, column=1)
        submit=Button(second, text="Submit", command=lambda:getEntry(epc,sheet,second,cpoint))
        submit.grid(row=0, column =2)
        second.mainloop()
        second.bind('<Return>',key())
        ax.plot(x, y,"r.") 
        ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
        Figscatter.draw()
        second.destroy()
        cpoint=cpoint+1
               
    ax.plot(x, y,"r.") 
    ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
    Figscatter.draw()

keyspressed = 0

def key():
    global keyspressed
    print(keyspressed)
    keyspressed += 1
    
def getEntry(myEntry,sheet,second,cpoint):
    res = myEntry.get()
    sheet.write(cpoint, 3, res)
    second.quit()
    return res


###############################################################################
###############################################################################
###############################################################################
## Main prog
###############################################################################
###############################################################################
###############################################################################

root = tk.Tk()

tk.Button(text='Quit', 
          command=close_window,
          fg="red").pack(side=tk.TOP, padx=10)

tk.Button(root, text="record",
          command=record).pack(side=tk.TOP, padx=10)

greeting = tk.Label(text="plan musée cap d'agde").pack(side=tk.TOP, padx=10)

openExpoFiles()

root.bind('<Button-1>', onclick)

root.mainloop()
