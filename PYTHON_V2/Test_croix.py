import xlrd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
from tkinter import *
import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import time, threading
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
init_angle=180
workbook=[]
sheet=[]
sw_record=0
cpoint=0
Hsize=400
Wsize=500
WAIT_SECONDS = 0.2
istep = 0
flag_update = 0
   
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
    image = mpimg.imread("plan_expo.png")
    image=np.flipud(image)
    #########
    
    ######### excel fichier acquisition
    document = xlrd.open_workbook("test_musee_EphebeV2.xls")
    feuille_1 = document.sheet_by_index(0)
    #Nb colonnes + lignes
    cols = feuille_1.ncols
    rows = feuille_1.nrows
    #########
    
    ######### excel fichier emplacement tags RFID
    document2 = xlrd.open_workbook("PosedeTagsMuseeAgde.xls")
    feuille_2 = document2.sheet_by_index(0)
    
    #############   
    figure = plt.Figure(figsize=(6,5), dpi=200)
    ax = figure.add_subplot(111)
    ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
    Figscatter = FigureCanvasTkAgg(figure, root) 
    Figscatter.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH)
    
def threadTrack():
    global istep, pts, flag_update, ax
    istep = istep + 1
    
    plotPosition(istep,0,0)
    
    flag_update=1
    
    print(time.ctime())
    
    if istep < len(pts)-1:
        threading.Timer(WAIT_SECONDS, threadTrack).start()            
    
    
def plotPosition(index,sw_ann,sw_clear):
    global ax, Figscatter, init_angle
    
    if sw_clear==1:
        ax.clear()       
    t = mpl.markers.MarkerStyle(marker=6)
    t._transform = t.get_transform().rotate_deg(Orientation[index]+init_angle)
    x = int(float(pts[index,0]))
    y = int(float(pts[index,1]))        
    ax.scatter(x, y, marker=t, color="red", s=70, alpha=0.5)
    ax.scatter(x, y, marker=".", color="black", s=50)    #marker=forme, s=epaisseur  
    if sw_ann==1:
        ann = add_annotation(pts[index,2], x, y)    

def processData():
    global Capteurs, OrientationData, Orientation, pts, ax, Figscatter, flag_update 
    pointFinal= []
    for i in range(1,rows):
        if feuille_1.cell_value(rowx=i,colx=0)=="Start datalogging..." or feuille_1.cell_value(rowx=i,colx=0)=="Stop datalogging...":
            i=i+1
            
        nbCapt=feuille_1.cell_value(rowx=i,colx=11)
        
        #saturation à 10 tags max
        if nbCapt>10:
            nbCapt=10
        
        capt = []
        for j in range(0,int(nbCapt)):
            capt += [[feuille_1.cell_value(rowx=i, colx=(j*2+12)), feuille_1.cell_value(rowx=i, colx=(j*2+13))]]

        Capteurs += [capt]
        
        x = float(feuille_1.cell_value(rowx=i, colx=8))
        y = float(feuille_1.cell_value(rowx=i, colx=9))-200
        z = float(feuille_1.cell_value(rowx=i, colx=10))
        OrientationData += [[x, y, z]]
                  
        ######On prend le capteur avec le plus grand RSSI
        RSSImax = -1
        Captmax = -1
        for element in capt:
            if element[0]<=254:
                if element[1]>RSSImax:
                    RSSImax = element[1]
                    Captmax = element[0]
        pointFinal += [Captmax]
        ######
    
    ########### calcul de l'orientation #################
    magneto = np.array(OrientationData)        
    magnetoY=magneto[:,1]
    magnetoZ=magneto[:,2]
    maxZ=max(magnetoZ)
    minZ=min(magnetoZ)
    maxY=max(magnetoY)
    minY=min(magnetoY)
    magYnorm=(magnetoY-minY)/(maxY-minY)*2-1
    magZnorm=(magnetoZ-minZ)/(maxZ-minZ)*2-1
    Orientation=np.arctan2(magYnorm, magZnorm) * 180 / np.pi;
#    Orientation=np.arccos(magZnorm)*180 / np.pi;
    
    print(Capteurs)
    print("pointFinal")
    print(pointFinal)

    ########## Récupération des coordonnées des tags
    coord = []
    for k in range (0, len(pointFinal)):
        if pointFinal[k]>0:
            coord += [[feuille_2.cell_value(rowx=int(pointFinal[k])-1, colx=1), feuille_2.cell_value(rowx=int(pointFinal[k])-1, colx=2),feuille_2.cell_value(rowx=int(pointFinal[k])-1, colx=3)]]       
    pts = np.array(coord)
    
    ########## Affichage du premier ######################
    plotPosition(0,0,1)
    flag_update=1

def plotTravel():
    global root, ax, Figscatter, istep           
    istep=0
    threadTrack()
        
def action1():
    print("The button was clicked!")
    plotTravel()
    
def plusAngle():
    global init_angle, flag_update
    init_angle+=5
    plotPosition(0,0,1)
    flag_update=1
    print(init_angle)
    
def minusAngle():
    global init_angle, flag_update
    init_angle-=5
    plotPosition(0,0,1)
    flag_update=1
    print(init_angle)
    
def save():
    global workbook, sheet, sw_record
    sw_record=0
    workbook.save("C:/Users/Arnaud/Documents/python/IHM Python/TAG_DATA.xls")
    
def record():
    global workbook, sheet, sw_record, cpoint
    cpoint=0
    workbook = xlwt.Workbook()
    sheet = workbook.add_sheet('feuille1')   
    sw_record=1
    
def onclick(event):
    global sw_record, sheet, cpoint, ax, Figscatter
    print("Single Click, Mouse position: (%s %s)" % (event.x, event.y))
    
    # inv = ax.transData.inverted()
    # val=inv.transform((event.x,  event.y))
    # print(val)
    # x=int(val[0])
    # y=int(400-val[1])
    x=event.x
    y=event.y

    if sw_record==1:
        sheet.write(cpoint, 0, cpoint)
        sheet.write(cpoint, 1,x)
        sheet.write(cpoint, 2, y)
        cpoint=cpoint+1
    
    
    # ax.plot(x, y,"r.") 
    # ax.plot(event.x, event.y,"r.") 
    # ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
    # Figscatter.draw()

###############################################################################
###############################################################################
###############################################################################
## Main prog
###############################################################################
###############################################################################
###############################################################################

root = tk.Tk()

monCadre = Frame(root,  bd=1, relief=SUNKEN) # Cadre placé dans la fenêtre
monCadre.pack(side=tk.TOP,padx=5, pady=5)


tk.Button(monCadre,text='Quit', 
          command=close_window,
          fg="red").pack(side=tk.LEFT,padx=5, pady=5)
tk.Button(monCadre, text="Analyse",
          command=action1).pack(side=tk.LEFT,padx=5, pady=5)

tk.Button(monCadre, text="+",
          command=plusAngle).pack(side=tk.LEFT,padx=5, pady=5)

tk.Button(monCadre, text="-",
          command=minusAngle).pack(side=tk.LEFT,padx=5, pady=5)

tk.Button(monCadre, text="record",
          command=record).pack(side=tk.LEFT,padx=5, pady=5)

tk.Button(monCadre, text="save",
          command=save).pack(side=tk.LEFT,padx=5, pady=5)

greeting = tk.Label(text="plan musée cap d'agde").pack(side=tk.TOP, padx=5, pady=5)

openExpoFiles()

processData()

root.bind('<Button-1>', onclick)

while True:
    if flag_update==1:
        ax.imshow(image,origin='lower',extent=[0,Wsize,0,Hsize])
        Figscatter.draw()
        flag_update=0;
    root.update()
















