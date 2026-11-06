# Generuje schemat polaczen (SVG + PNG). Uruchom: python make_schematic.py
# wygenerowane za pomocą Claude 
import html
W, H = 1500, 1130
o = []
def add(s): o.append(s)
def rect(x,y,w,h,fill="#fff",stroke="#333",sw=2,rx=8): add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
def text(x,y,t,size=15,anchor="start",weight="normal",fill="#111"): add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="{fill}" font-family="DejaVu Sans, Arial, sans-serif">{html.escape(t, quote=False)}</text>')
def line(x1,y1,x2,y2,color="#333",sw=3,dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ''
    add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{sw}"{d}/>')
def dot(x,y,c="#333"): add(f'<circle cx="{x}" cy="{y}" r="5" fill="{c}"/>')

add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
rect(0,0,W,H,"#fafafa","none",0,0)
text(W/2,38,"Akwarium ESP32 - schemat polaczen (wersja sprzetowa)",26,"middle","bold")
text(W/2,64,"Na rysunku tylko sygnaly. Zasilanie opisano na czerwono w ramkach urzadzen. Wszystkie masy (GND) lacz razem.",14,"middle",fill="#555")

# --- ESP32 ---
ex,ey,ew,eh = 620,150,260,760
rect(ex,ey,ew,eh,"#e8f0fe","#1a73e8",3,12)
text(ex+ew/2,ey+34,"ESP32 DevKitC",22,"middle","bold")
text(ex+ew/2,ey+58,"(WROOM-32, 38 pin)",14,"middle",fill="#555")
text(ex+ew/2,ey+eh-44,"Zasilanie: pin 5V/VIN",13,"middle",fill="#c5221f")
text(ex+ew/2,ey+eh-24,"Logika czujnikow: 3,3 V",13,"middle",fill="#c5221f")

def pin_left(y,label,color):
    line(ex-60,y,ex,y,color); dot(ex,y,color); text(ex+10,y+5,label,14,"start","bold")
def pin_right(y,label,color):
    line(ex+ew,y,ex+ew+60,y,color); dot(ex+ew,y,color); text(ex+ew-10,y+5,label,14,"end","bold")

# --- LEWA STRONA: czujniki analogowe, cisnienie, przekaznik ---
lx,lw = 40,490
def dev_left(y,h,title,lines_,fill="#fff"):
    rect(lx,y,lw,h,fill,"#333",2,8); text(lx+14,y+26,title,17,"start","bold")
    for i,t in enumerate(lines_):
        col = "#c5221f" if t.startswith("zasil") else "#333"
        text(lx+14,y+50+i*20,t,13,"start",fill=col)

# TDS
y=250; dev_left(y-50,120,"Czujnik TDS (DFRobot SEN0244)",["zasilanie: 3,3 V i GND","wyjscie analogowe 0-2,3 V -> GPIO32 (ADC1)","filtr: srednia kroczaca N=10"])
pin_left(y,"GPIO32","#2e7d32"); line(lx+lw,y,ex-60,y,"#2e7d32")
# zmetnienie
y=410; dev_left(y-60,150,"Czujnik zmetnienia (SEN0189)",["zasilanie: 5 V i GND (wyjscie do ~4,5 V!)","wyjscie -> dzielnik 10k / 10k (1:1) -> GPIO33","100 nF miedzy GPIO33 a GND (przy pinie)","dzielnik OBOWIAZKOWY: ADC znosi max 3,3 V"],"#fff8e1")
pin_left(y,"GPIO33","#ef6c00"); line(lx+lw,y,ex-60,y,"#ef6c00")
# cisnienie
y=585; dev_left(y-65,175,"Cisnienie: MPS20N0040D + modul HX710B",["zasilanie modulu: 3,3 V (NIE 5 V) i GND","OUT (DOUT) -> GPIO34 (wejscie, input-only)","SCK -> GPIO26 (wyjscie)","interfejs cyfrowy 24-bit, nie ADC","wezyk powietrza podlaczony do czujnika"])
pin_left(y-15,"GPIO34","#6a1b9a"); line(lx+lw,y-15,ex-60,y-15,"#6a1b9a")
pin_left(y+20,"GPIO26","#6a1b9a"); line(lx+lw,y+20,ex-60,y+20,"#6a1b9a")
# przekaznik
y=790; dev_left(y-55,150,"Modul przekaznika Iduino 10 A (opto)",["zasilanie: VCC 5 V i GND","IN <- GPIO25 (sterowanie)","polaryzacja: do sprawdzenia (RELAY_ACTIVE_HIGH)","wyjscie sieciowe: COM / NO -> zob. ramka 230 V"],"#fdecea")
pin_left(y,"GPIO25","#c5221f"); line(lx+lw,y,ex-60,y,"#c5221f")

# --- PRAWA STRONA: 1-Wire i I2C ---
rx,rw = 970,490
def dev_right(y,h,title,lines_,fill="#fff"):
    rect(rx,y,rw,h,fill,"#333",2,8); text(rx+14,y+26,title,17,"start","bold")
    for i,t in enumerate(lines_):
        col = "#c5221f" if t.startswith("zasil") else "#333"
        text(rx+14,y+50+i*20,t,13,"start",fill=col)
y=250; dev_right(y-50,130,"Czujnik temperatury DS18B20 (sonda)",["zasilanie: 3,3 V i GND","DQ (zolty) -> GPIO4","rezystor 4,7 kohm: DQ <-> 3,3 V (pull-up)"])
pin_right(y,"GPIO4","#1565c0"); line(ex+ew+60,y,rx,y,"#1565c0")
# I2C
y1,y2=470,510
rect(rx,390,rw,230,"#fff","#333",2,8)
text(rx+14,416,"Magistrala I2C (jedna dla obu urzadzen)",17,"start","bold")
text(rx+14,442,"zasilanie obu: 3,3 V i GND",13,"start",fill="#c5221f")
text(rx+14,492,"Wyswietlacz OLED 0,96\" SSD1306  - adres 0x3C",14,"start","bold")
text(rx+14,512,"(czasem 0x3D - sprawdz skanerem I2C)",12,"start",fill="#555")
text(rx+14,550,"Czujnik swiatla BH1750 (GY-302)  - adres 0x23",14,"start","bold")
text(rx+14,570,"ADDR do GND; podciagniecia SDA/SCL zwykle na modulach",12,"start",fill="#555")
pin_right(y1,"GPIO21 (SDA)","#00897b"); line(ex+ew+60,y1,rx,y1,"#00897b")
pin_right(y2,"GPIO22 (SCL)","#00897b"); line(ex+ew+60,y2,rx,y2,"#00897b")

# --- ZASILANIE ---
rect(ex+30,ey-85,200,60,"#fff","#c5221f",2,8)
text(ex+130,ey-58,"Zasilacz Akyga",14,"middle","bold"); text(ex+130,ey-38,"5 V / 2 A",14,"middle")
line(ex+130,ey-25,ex+130,ey,"#c5221f",4); dot(ex+130,ey,"#c5221f")
text(ex+142,ey-8,"5V/VIN",12,"start",fill="#c5221f")
text(40,ey-62,"Szyna 5 V: przekaznik VCC, zmetnienie VCC",13,"start",fill="#c5221f")
text(40,ey-42,"Szyna 3,3 V (pin 3V3): DS18B20, TDS, HX710B, OLED, BH1750",13,"start",fill="#c5221f")

# --- 230 V ---
bx,by,bw,bh = 40,935,1420,185
rect(bx,by,bw,bh,"#fff0f0","#c5221f",3,10)
text(bx+16,by+28,"UWAGA - OBWOD 230 V (niebezpieczny dla zycia). Pracuj tylko przy odlaczonej wtyczce. Jesli nie masz doswiadczenia - popros osobe z uprawnieniami.",15,"start","bold","#c5221f")
# schemat obwodu
cx=bx+60; cy=by+90
text(cx-20,cy-30,"Wtyczka 230 V",13,"start","bold")
line(cx,cy,cx+130,cy,"#6d4c41",4); text(cx+10,cy-8,"L (faza)",12)
rect(cx+130,cy-18,90,36,"#fff","#333",2,4); text(cx+175,cy+5,"bezpiecznik",12,"middle")
line(cx+220,cy,cx+420,cy,"#6d4c41",4)
rect(cx+420,cy-30,200,60,"#fdecea","#333",2,6); text(cx+520,cy-6,"przekaznik",13,"middle","bold"); text(cx+520,cy+14,"COM -> NO (styki)",12,"middle")
line(cx+620,cy,cx+820,cy,"#6d4c41",4)
rect(cx+820,cy-30,160,60,"#fff","#333",2,6); text(cx+900,cy-6,"Grzalka",13,"middle","bold"); text(cx+900,cy+14,"akwarium",12,"middle")
line(cx,cy+40,cx+820,cy+40,"#1565c0",4); text(cx+10,cy+34,"N (zero) - prosto do grzalki, BEZ przekaznika",12)
line(cx+820,cy+40,cx+900,cy+40,"#1565c0",4); line(cx+900,cy+30,cx+900,cy+40,"#1565c0",4)
text(bx+16,by+bh-42,"PE (uziemienie): lacz do grzalki i obudowy metalowej. Zalecane: grzalka z wlasnym termostatem (ostatnia linia obrony),",12)
text(bx+16,by+bh-22,"obudowa z odciazeniem kabli i bezpiecznik dobrany do mocy grzalki (I = P / 230 V, zapas ok. 50%).",12)
add('</svg>')
svg="\n".join(o)
open("schemat_polaczen.svg","w",encoding="utf-8").write(svg)
import cairosvg
cairosvg.svg2png(bytestring=svg.encode("utf-8"),write_to="schemat_polaczen.png",output_width=1500)
print("ok")
