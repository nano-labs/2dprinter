#! /usr/bin/env python
import os
from sys import argv
from PIL import Image
from math import ceil
from xml.dom import minidom
import serial
from time import sleep
from tqdm import tqdm


class SVG:

    def __init__(self, svg_file, bed_size=(10840, 7320)):
        doc = minidom.parse(svg_file)
        path_strings = [path.getAttribute('d') for path in doc.getElementsByTagName('path')]
        self.paths = []
        self.bed_size = bed_size
        zoom = 1.0
        for path_string in path_strings:
            p = Path(path_string, zoom=zoom)
            self.paths.append(p)
        size = self.get_size()
        zoom = min(bed_size[0] / size[0], bed_size[1] / size[1])
        self.paths = []
        for path_string in path_strings:
            p = Path(path_string, zoom=zoom)
            self.paths.append(p)

    def get_size(self):
        xs, ys = [], []
        for p in self.paths:
            for x, y in p._all:
                xs.append(x)
                ys.append(y)
        self.max_x, self.max_y = ceil(max(xs)), ceil(max(ys))
        size = (self.max_x, self.max_y)
        return size

    def draw(self):
        size = self.get_size()
        output = Image.new("RGB", size, color=(255, 255, 255))
        for p in self.paths:
            output = p.draw(output)
        output.show()

    def plot(self):
        def find_serial_port():
            for p in os.listdir("/dev/"):
                if p.startswith("cu.usb"):
                    return "/dev/{}".format(p)

        ser = serial.Serial(find_serial_port(), 115200, timeout=1)
        sleep(3)

        r = ser.readline()
        print("Waiting serial connection", end='')
        while not r == b'started\r\n':
            sleep(1)
            print(".", end='')
            r = ser.readline()

        # print("\nReseting params", end = '')
        # ser.write(bytes("R;\r\n".encode()))
        # while not ser.readline() == b'ack\r\n':
        #     sleep(1)
        #     print(".", end = '')
        print("\nStarting")

        commands = ["R"]
        for p in self.paths:
            # output = p.plot(ser)
            commands.extend(p.commands)

        # Com Borda
        # commands.append("M0.0,{}.0".format(self.max_y))
        # commands.append("L0.0,0.0")
        # commands.append("L{}.0,0.0".format(self.max_x))
        # commands.append("L{}.0,{}.0".format(self.max_x, self.max_y))
        # commands.append("L0.0,{}.0".format(self.max_y))
        # Sem borda
        commands.append("M0.0,{}.0".format(self.bed_size[1]))

        messages = []
        for command in tqdm(commands):
            # print(command)
            messages.append(command)
            if len(messages) >= 5:
                payload = "{};\r\n".format(";".join(messages))
                messages = []
                ser.write(bytes(payload.encode()))
                while not ser.readline() == b'ack\r\n':
                    pass
        if messages:
            payload = "{};\r\n".format(";".join(messages))
            ser.write(bytes(payload.encode()))
            while not ser.readline() == b'ack\r\n':
                pass
        ser.close()


class Path:
    def __init__(self, path_string, zoom):
        self._all = []
        self.pos_x = 0.0
        self.pos_y = 0.0
        self.start = None
        self.zoom = zoom
        self.commands = []
        self.parse_d(path_string)

    def parse_d(self, d):
        def parse_command(command_string):
            if not command_string:
                return
            command = command_string[0]
            if command in "Zz":
                return self.Z()
            args = command_string[1:]
            args = args.strip()
            if args.startswith(" "):
                args = args[1:]
            args = args.replace("  ", " ").replace(" ", ",")
            args = [float(f) for f in args.split(",")]
            args = [a * self.zoom for a in args]
            # if command == "M":
            #     return self.M(*args)
            if command == "M":
                pen_up = True
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 2:
                        if pen_up:
                            self.M(*subargs)
                            pen_up = False
                        else:
                            self.L(*subargs)
                        subargs = []

            elif command == "m":
                pen_up = True
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 2:
                        if pen_up:
                            self.m(*subargs)
                            pen_up = False
                        else:
                            self.l(*subargs)
                        subargs = []

            elif command == "h":
                for a in args:
                    self.h(a)
            elif command == "H":
                for a in args:
                    self.H(a)
            elif command == "v":
                for a in args:
                    self.v(a)
            elif command == "V":
                for a in args:
                    self.V(a)

            elif command == "L":
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 2:
                        self.L(*subargs)
                        subargs = []
            elif command == "l":
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 2:
                        self.l(*subargs)
                        subargs = []

            elif command == "C":
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 6:
                        self.C(*subargs)
                        subargs = []
            elif command == "c":
                subargs = []
                for a in args:
                    subargs.append(a)
                    if len(subargs) == 6:
                        self.c(*subargs)
                        subargs = []

        command = ""
        for l in d:
            if l in "MmLlHhVvCcZzq":
                parse_command(command)
                command = l
            else:
                command += l
        parse_command(command)

    def goto_xy(self, x, y):
        self._all.append((x, y))
        self.pos_x = x
        self.pos_y = y

    def cubic_bezier(self, x0, y0, x1, y1, x2, y2, x3, y3):
        delta = int(max(abs(x3 - x0), abs(y3 - y0)) * 2)
        for t in range(delta):
            t = t / delta
            x = (
                (1 - t) ** 3 * x0
                + 3 * (1 - t) ** 2 * t * x1
                + 3 * (1 - t) * t ** 2 * x2
                + t ** 3 * x3
            )
            y = (
                (1 - t) ** 3 * y0
                + 3 * (1 - t) ** 2 * t * y1
                + 3 * (1 - t) * t ** 2 * y2
                + t ** 3 * y3
            )
            self.goto_xy(x, y)

    def m(self, x1, y1):
        return self.M(self.pos_x + x1, self.pos_y + y1)

    def M(self, x0, y0):
        if not self.start:
            self.start = (x0, y0)
        self.goto_xy(x0, y0)
        self.commands.append("M{:.1f},{:.1f}".format(x0, y0))

    def h(self, x1):
        return self.H(self.pos_x + x1)

    def H(self, x1):
        x0 = self.pos_x
        dx = ceil(x1 - x0)
        if dx > 0:
            for x in range(dx):
                self.goto_xy(x0 + x, self.pos_y)
        else:
            for x in range(abs(dx)):
                self.goto_xy(x0 - x, self.pos_y)
        self.commands.append("H{:.1f}".format(x1))

    def v(self, y1):
        return self.V(self.pos_y + y1)

    def V(self, y1):
        y0 = self.pos_y
        dy = ceil(y1 - y0)
        if dy > 0:
            for y in range(dy):
                self.goto_xy(self.pos_x, y0 + y)
        else:
            for y in range(abs(dy)):
                self.goto_xy(self.pos_x, y0 - y)
        self.commands.append("V{:.1f}".format(y1))

    def c(self, x1, y1, x2, y2, x3, y3):
        self.C(
            self.pos_x + x1,
            self.pos_y + y1,
            self.pos_x + x2,
            self.pos_y + y2,
            self.pos_x + x3,
            self.pos_y + y3,
        )

    def C(self, x1, y1, x2, y2, x3, y3):
        # self.start = (self.pos_x, self.pos_y)
        self.cubic_bezier(self.pos_x, self.pos_y, x1, y1, x2, y2, x3, y3)
        self.commands.append(
            "C{:.1f},{:.1f},{:.1f},{:.1f},{:.1f},{:.1f}".format(x1, y1, x2, y2, x3, y3)
        )

    def l(self, x1, y1):
        return self.L(self.pos_x + x1, self.pos_y + y1)

    def L(self, x1, y1):
        x0 = self.pos_x
        y0 = self.pos_y
        dx = x1 - x0
        dy = y1 - y0
        if dx == 0.0:
            return self.V(y1)
        elif dy == 0.0:
            return self.H(x1)
        else:
            if x1 > x0:
                for mx in range(int(x0), int(x1)):
                    my = y0 + dy * (mx - x0) / dx
                    self.goto_xy(mx, my)
            else:
                for mx in range(int(x0), int(x1), -1):
                    my = y0 + dy * (mx - x0) / dx
                    self.goto_xy(mx, my)
            self.goto_xy(x1, y1)
        self.commands.append("L{:.1f},{:.1f}".format(x1, y1))

    def Z(self):
        self.L(*self.start)
        self.start = None
        self.commands.append("Z")

    def draw(self, output):
        color = (0, 0, 0)
        for x, y in self._all:
            try:
                output.putpixel((int(x), int(y)), color)
            except:
                pass
        for command in self.commands:
            print("{}({});".format(command[0], command[1:]))

        return output

    # def plot(self, ser):
    #     messages = []
    #     for command in self.commands:
    #         print(command)
    #         messages.append(command)
    #         if len(messages) >= 10:
    #             payload = "{};\r\n".format(";".join(messages))
    #             messages = []
    #             ser.write(bytes(payload.encode()))
    #             while not ser.readline() == b'ack\r\n':
    #                 pass
    #     if messages:
    #         payload = "{};\r\n".format(";".join(messages))
    #         ser.write(bytes(payload.encode()))
    #         while not ser.readline() == b'ack\r\n':
    #             pass


if __name__ == "__main__":
    file_path = argv[1]
    s = SVG(file_path)
    s.draw()
    s.plot()
