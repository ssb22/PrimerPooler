#!/usr/bin/env python
# (works in either Python 2 or Python 3)

# Read timing and output files from Unix "script" utility
# and convert them to modified-DOStoy input for HTML5 canvas.
# (c) 2016,2020 Silas S. Brown.  Version 1.31.
# License: MIT as per the modified DOStoy we include (see below)

# use: script -t log2 2>log1 (GNU/Linux, not BSD/Mac as it doesn't log the timing)

# Where to find history:
# on GitHub at https://github.com/ssb22/PrimerPooler
# and on GitLab at https://gitlab.com/ssb22/PrimerPooler
# and on BitBucket https://bitbucket.org/ssb22/primerpooler
# and at https://gitlab.developers.cam.ac.uk/ssb22/PrimerPooler
# and in China: https://gitee.com/ssb22/PrimerPooler

timingFile = open("log1")
outputFile = open("log2")

import re ; bg,fg,bright = 0,7,0
dn = [(int(float(delay)*1000),int(nChars)) for delay,nChars in [l.split() for l in timingFile.read().split("\n") if l]]
i=0 ; minDelay,maxDelay = 100,3000
while i<len(dn)-1:
    while dn[i][0] < minDelay and dn[i+1][0] < minDelay:
        dn[i] = (dn[i][0]+dn[i+1][0],dn[i][1]+dn[i+1][1])
        del dn[i+1]
        if i >= len(dn)-1: break
    i += 1
try:
    xrange # Python 2
    def S(m): return m
except: # Python 3
    xrange = range
    def S(m): return m.decode('latin1')
    _,outputFile = outputFile,outputFile.buffer
outputFile.readline() # ignore start
for i in xrange(len(dn)): dn[i] = (dn[i][0],outputFile.read(dn[i][1]))
del outputFile ; i = 0
newline = u"\n".encode('utf-8') # Python 2 or Python 3
backslashR = u"\r".encode('utf-8') # Python 2 or Python 3
tab = u"\t".encode('utf-8') # Python 2 or Python 3
empty = u"".encode('utf-8') # Python 2 or Python 3
while i<len(dn):
    while len(dn[i][1].split(newline)) > 25:
        # More than 1 screen at a time is bad news: it won't be displayed, and overloading the JS interpreter can completely crash some tablet browsers.  Break it up a bit.
        dn.insert(i,(dn[i][0],newline.join(dn[i][1].split(newline)[:25])+newline))
        i += 1
        dn[i] = (minDelay,newline.join(dn[i][1].split(newline)[25:]))
    i += 1
funcNo = 1
print ("""
/**
 * dostoy - A silly interactive retro console library for the HTML5 canvas
 * Originally from https://github.com/toolsley/dostoy
 * Cut down by Silas S. Brown: Removed input and shell parts;
 * embed font; add backspace(), clearEOL() etc; changed
 * colour order to ANSI rather than qbasic; write own canvas
 * and check for browser support and screen-width first.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Toolsley.com
 * Changes copyright (c) 2016 Silas S. Brown
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

if(screen && screen.width>=800 && document.getElementById && "getContext" in document.createElement('canvas')) {

var dostoy = function () {

    var charSet = [];
    var fontHeight = 0;

    var curX = 0;
    var curY = 0;

    var maxX = 0;
    var maxY = 0;

    var foregroundColor = 7;
    var backgroundColor = 0;

    var ctx = null;

    var cursor = true;
    var flipflop = true;

    var ansiColors = [ // quickbasic order
        [0, 0, 0],
        [170, 0, 0],
        [0, 170, 0],
        [170, 85, 0],
        [0, 0, 170],
        [170, 0, 170],
        [0, 170, 170],
        [170, 170, 170],
        [85, 85, 85],
        [255, 85, 85],
        [85, 255, 85],
        [255, 255, 85],
        [85, 85, 255],
        [255, 85, 255],
        [85, 255, 255],
        [255, 255, 255]
    ];

    /**
     * index starts at the first printable keyCode (code 48, char "0")
     **/

    var byte2bits = function (a) {
        var tmp = "";
        for (var i = 128; i >= 1; i /= 2)
            tmp += a & i ? '1' : '0';
        return tmp;
    }

    var initCharSet = function (font, width) {
        var fontBuffer = null;
        if (typeof font == "Uint8Array") {
            fontBuffer = font;
        } else {
            var rawLength = font.length;
            fontBuffer = new Uint8Array(new ArrayBuffer(rawLength));
            for (i = 0; i < rawLength; i++) {
                fontBuffer[i] = font.charCodeAt(i);
            }
        }
        for (var x = 0; x < fontBuffer.length / width; x++) {
            var charI = ctx.createImageData(8, width);
            for (var i = 0; i < width; i++) {
                var bitString = byte2bits(fontBuffer[(x * width) + i]);
                for (var j = 0; j < 8; j++) {
                    charI.data[((i * 8) + j) * 4 + 0] = bitString[j] == "1" ? 255 : 0;
                    charI.data[((i * 8) + j) * 4 + 1] = bitString[j] == "1" ? 255 : 0;
                    charI.data[((i * 8) + j) * 4 + 2] = bitString[j] == "1" ? 255 : 0;
                    charI.data[((i * 8) + j) * 4 + 3] = 255;
                }
            }
            charSet.push(charI);
        }
    }

    var cls = function () {
        curX = 0;
        curY = 0;
        ctx.rect(0, 0, ctx.canvas.width, ctx.canvas.height);
        ctx.fillStyle = "rgba(" + ansiColors[backgroundColor][0] + "," + ansiColors[backgroundColor][1] + "," + ansiColors[backgroundColor][2] + ",1)";
        ctx.fill();

    }
    var initCanvas = function (canvas) {
        ctx = canvas.getContext("2d");
        cls();
    }

    var chr = function (code) {
        var codes = code.split(",");
        var out = "";
        for (var i = 0, icode; icode = codes[i]; i++)
            out += String.fromCharCode(icode);

        return out;
    }

    var newLine = function () {
        cursorBlink(false);
        if (curY + 1 > maxY) {
            ctx.putImageData(ctx.getImageData(0, fontHeight, ctx.canvas.width, ctx.canvas.height - fontHeight), 0, 0);
            ctx.rect(0, maxY * fontHeight, ctx.canvas.width, fontHeight);
            ctx.fillStyle = "rgba(0,0,0,1)";
            ctx.fill();

            curX = 0;

        } else {
            curY++;
            curX = 0;
        }
    }

    var print = function (text) {
        text = text.toString();
        var startXReal = curX * 8;
        var startYReal = curY * fontHeight;

        for (var i = 0; i < Math.min(text.length, maxX); i++) {

            var charImage = charSet[text.charCodeAt(i)];

            if (backgroundColor != 0 || foregroundColor != 15) {

                var colorizedCharImage = ctx.createImageData(8, fontHeight);

                for (var j = 0; j < colorizedCharImage.data.length; j += 4) {
                    colorizedCharImage.data[j] = charImage.data[j] !== 255 ? ansiColors[backgroundColor][0] : ansiColors[foregroundColor][0];
                    colorizedCharImage.data[j + 1] = charImage.data[j + 1] !== 255 ? ansiColors[backgroundColor][1] : ansiColors[foregroundColor][1];
                    colorizedCharImage.data[j + 2] = charImage.data[j + 2] !== 255 ? ansiColors[backgroundColor][2] : ansiColors[foregroundColor][2];
                    colorizedCharImage.data[j + 3] = 255;
                }

                ctx.putImageData(colorizedCharImage, startXReal + (i * 8), startYReal);
            } else {
                ctx.putImageData(charImage, startXReal + (i * 8), startYReal);
            }

            curX++;
        }
    }

    var color = function (bg, fg) {
        backgroundColor = bg;
        foregroundColor = fg;
    }

    var println = function (text) {
        if (text)
            print(text);
        newLine();
    }

    var cursorBlink = function (forceState) {
        if (cursor) {
            var XReal = curX * 8;
            var YReal = curY * fontHeight;
            if (typeof forceState != "undefined") flipflop = forceState;

            if (flipflop) {
                ctx.strokeStyle = "rgba(255,255,255,1)";
                ctx.beginPath();
                ctx.lineWidth = 2;
                ctx.moveTo(XReal, YReal + 13);
                ctx.lineTo(XReal + 8, YReal + 13);
                ctx.stroke();

                flipflop = false;

            } else {
                ctx.strokeStyle = "rgba(0,0,0,1)";
                ctx.beginPath();
                ctx.lineWidth = 2;
                ctx.moveTo(XReal, YReal + 13);
                ctx.lineTo(XReal + 80, YReal + 13); // ugly hack to clear any cursor remnants
                ctx.stroke();

                flipflop = true;
            }

        }
    }

    var setCursor = function (state) {
        cursor = state;
    }

    var init = function (config) {
        if (!config.canvas) throw "canvas missing from config";
        initCanvas(config.canvas);
        initCharSet(window.atob("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AAAYGBgYGBgY/wAAAAAAAAAAAAAAAAD/GBgYGBgYGBgYGBgYGB8YGBgYGBgAAAAAAAAA/wAAAAAAABgYGBgYGBj/GBgYGBgYGBgYGBgfGB8YGBgYGBg2NjY2NjY2NzY2NjY2NjY2NjY2NzA/AAAAAAAAAAAAAAA/MDc2NjY2NjY2NjY2NvcA/wAAAAAAAAAAAAAA/wD3NjY2NjY2NjY2NjY3MDc2NjY2NjYAAAAAAP8A/wAAAAAAADY2NjY29wD3NjY2NjY2GBgYGBj/AP8AAAAAAAA2NjY2NjY2/wAAAAAAAAAAAAAA/wD/GBgYGBgYAAAAAAAAAP82NjY2NjY2NjY2NjY2PwAAAAAAABgYGBgYHxgfAAAAAAAAAAAAAAAfGB8YGBgYGBgAAAAAAAAAPzY2NjY2NjY2NjY2Njb/NjY2NjY2GBgYGBj/GP8YGBgYGBgYGBgYGBgY+AAAAAAAAAAAAAAAAAAfGBgYGBgY//////////////////8AAAAAAAAA//////////Dw8PDw8PDw8PDw8PDwDw8PDw8PDw8PDw8PDw//////////AAAAAAAAAAAAAAAAdtzY2Nx2AAAAAAAAAHzG/MbG/MDAQAAAAP7GxsDAwMDAwAAAAAAAAAD+bGxsbGxsAAAAAAD+xmAwGDBgxv4AAAAAAAAAAH7Y2NjYcAAAAAAAAABmZmZmfGBgwAAAAAAAAHbcGBgYGBgAAAAAAH4YPGZmZjwYfgAAAAAAOGzGxv7Gxmw4AAAAAAA4bMbGxmxsbO4AAAAAAB4wGAw+ZmZmPAAAAAAAAAAAftvbfgAAAAAAAAADBn7b2/N+YMAAAAAAABwwYGB8YGAwHAAAAAAAAHzGxsbGxsbGAAAAAAAA/gAA/gAA/gAAAAAAAAAYGH4YGAAA/wAAAAAAMBgMBgwYMAB+AAAAAAAMGDBgMBgMAH4AAAAAAA4bGxgYGBgYGBgYGBgYGBgYGBgY2NhwAAAAAAAAGBgAfgAYGAAAAAAAAAAAdtwAdtwAAAAAAAA4bGw4AAAAAAAAAAAAAAAAAAAAGBgAAAAAAAAAAAAAAAAAGAAAAAAAAAAPDAwMDAzsbDwcAAAAANhsbGxsbAAAAAAAAAAAcNgwYMj4AAAAAAAAAAAAAAB8fHx8fHwAAAAAAAAAAAAAAAAAAAAAAAA="),14);
        fontHeight = 14;

        maxY = Math.floor(ctx.canvas.height / fontHeight) - 1;
        maxX = Math.floor(ctx.canvas.width / 8) - 1;
        window.setInterval(cursorBlink, 500);
    }

    var locate = function (col, row) {
        if (col >=0)
            curX = col <= maxX ? col : 0;
        if (row >=0)
            curY = row <= maxY ? row : 0;
    }
    var backspace = function() {
        if(curX>0) curX--;
        else if(curY>0) locate(maxX-1,curY-1);
        else locate(0,0);
    }
    var clearEOL = function() {
        var x=curX,y=curY;
        while(curX<maxX-1) print(' '); print(' ');
        curX=x; curY=y;
    }
    var startOfLine = function() {
        curX = 0;
    }

    return {
        init: init,
        print: print,
        println: println,
        locate: locate, backspace: backspace,
        clearEOL: clearEOL, startOfLine:startOfLine,
        cls: cls,
        chr: chr,
        color: color,
        setCursor: setCursor,
        getCols: function(){return maxX;},
        getRows: function(){return maxY;}
    }
}();
document.getElementById('DOStoyPlace').innerHTML='<canvas id="DOStoyCanvas" width="640" height="350" style="border:thin blue solid;margin:1ex;float:right"></canvas>';
window.setTimeout(function () {
dostoy.init({canvas:document.getElementById("DOStoyCanvas")});
    dostoy.setCursor(false);
    var repeat = function (text, num) {
        return new Array(num + 1).join(text);
    };
    for(var i=0; i<9; i++) dostoy.println();
    dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
                dostoy.println(dostoy.chr("201" + repeat(",205", 34) + ",187"));
    dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
                dostoy.println(dostoy.chr("186") + "  Click to view a demonstration   " + dostoy.chr("186"))
    dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
                dostoy.println(dostoy.chr("200" + repeat(",205", 34) + ",188"));
                dostoy.color(0, 7);
                dostoy.println();
    var Canvas = document.getElementById("DOStoyCanvas");
    function onclick(){
        Canvas.removeEventListener("click",onclick);
        function pause() { if(window.dosToyPaused){window.dosToyPaused=false; dostoy.setCursor(true);if(window.dosToyNext)window.dosToyNext()}else{window.dosToyNext=false;window.dosToyPaused=true;dostoy.setCursor(false)}}
        Canvas.addEventListener("click",pause);
        dostoy.cls(); dostoy.setCursor(true);
function ST(t,f) { window.setTimeout(function(){if(window.dosToyPaused) window.dosToyNext=f; else f()},t); }
function f0() {""")
for delay,chars in dn:
    if delay < minDelay: delay = 0
    if delay > maxDelay: delay = maxDelay
    if chars.endswith(backslashR) or chars.startswith(backslashR):
        delay = min(delay,minDelay)
    if delay:
        print ("ST("+str(delay)+",f"+str(funcNo)+")} function f"+str(funcNo)+"(){")
        funcNo += 1
    x=0 ; width=80
    while chars:
        if ord(chars[0:1])==8:
            print ("dostoy.backspace();")
            x -= 1
            if x<0: x = width-1
            chars = chars[1:] ; continue
        if chars.startswith(u'\x1b[K'.encode('utf-8')):
            print ("dostoy.clearEOL();")
            chars = chars[3:] ; continue
        if chars.startswith(u'\x1b]0;'.encode('utf-8')): # TODO check this
            chars = chars[4:] ; continue
        m = re.match(re.escape(u"\x1b[([0-9;]*)m".encode('utf-8')),chars)
        if m:
            for code in m.group(1).split(u';'.encode('utf-8')):
                code = int(code)
                if 30<=code<=37: fg = code-30
                elif 40<=code<=47: bg=code-40
                elif code==1: bright = 8
                elif code==0: bg,fg,bright = 0,7,0
            print ("dostoy.color(%d,%d);" % (bg,fg+bright))
            chars = chars[m.end():] ; continue
        m = re.match(u'[ -~]*(\\r?\\n)?'.encode('utf-8'),chars)
        if m and m.end():
            m = m.group()
            l = len(m.replace(backslashR,empty))
            if l+x >= width and not (l+x==width and m.endswith(newline)):
                m=m[:width-x].replace(backslashR,empty)
                f = "println"
            elif m.endswith(newline): f = "println"
            else: f="print"
            chars = chars[len(m):]
            if m.endswith(newline): m = m[:-1].replace(backslashR,empty)
            print ("dostoy."+f+"(\""+S(m).replace('\\','\\\\').replace('"','\\"')+"\");")
            if f=="print": x += len(m)
            else: x = 0
        elif chars[0:1]==backslashR:
            print ("dostoy.startOfLine();")
            chars = chars[1:] ; x = 0
        elif chars[0:1]==tab:
            chars = ' '*(8-(x%8)) + chars[1:]
        else:
            print ("/* Ignoring "+repr(chars[:1])+" */")
            chars = chars[1:]
print ("""ST(3000,function(){dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
dostoy.println(dostoy.chr("201" + repeat(",205", 34) + ",187"));
dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
dostoy.println(dostoy.chr("186") + "   *** End of demonstration ***   " + dostoy.chr("186"))
dostoy.color(0,7); dostoy.print(repeat(" ",20)); dostoy.color(4, 15);
dostoy.println(dostoy.chr("200" + repeat(",205", 34) + ",188"));
dostoy.setCursor(false); dostoy.color(0,7);
Canvas.removeEventListener("click",pause);
Canvas.addEventListener("click",onclick);})}f0();
        return false;
            };
    Canvas.addEventListener("click",onclick);
},1000);}
""")
