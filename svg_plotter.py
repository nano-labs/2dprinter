#! /usr/bin/env python
import os
from decimal import Decimal
from io import BytesIO
from math import ceil, floor
from sys import argv
from time import sleep

from PIL import Image
from xml.dom import minidom
import serial
from tqdm import tqdm
from cairosvg import svg2png


class SVG:
    # def __init__(self, svg_file, bed_size=(10840, 7320)):
    def __init__(self, svg_file, bed_size=(Decimal(1084), Decimal(732))):
        # def __init__(self, svg_file, bed_size=(5840, 3620)):
        self.svg_file = svg_file
        doc = minidom.parse(svg_file)
        path_strings = [path.getAttribute('d') for path in doc.getElementsByTagName('path')]
        self.paths = []
        self.bed_size = bed_size
        zoom = Decimal(1.0)
        for path_string in path_strings:
            p = Path(path_string, zoom=zoom)
            self.paths.append(p)
        self.size = self.get_size()
        zoom = min(bed_size[0] / self.size[0], bed_size[1] / self.size[1])
        self.paths = []
        for path_string in path_strings:
            p = Path(path_string, zoom=zoom)
            self.paths.append(p)
        self.size = self.get_size()
        self.infill_map = None

    def get_size(self):
        xs, ys = [], []
        for p in self.paths:
            for x, y in p._all:
                xs.append(x)
                ys.append(y)
        self.max_x, self.max_y = ceil(max(xs)), ceil(max(ys))
        self.size = (self.max_x + 1, self.max_y + 1)
        return self.size

    def draw(self):
        size = self.get_size()
        output = Image.new("RGB", size, color=(255, 255, 255))
        for p in self.paths:
            output = p.draw(output)
        output = self.make_infill_map(output)

        output = Image.new("RGB", size, color=(255, 255, 255))
        output = self.add_infill(output, density=30)
        for p in self.paths:
            output = p.draw(output)
        output.show()

    def make_infill_map(self, output):
        def get_min_image(im):
            """Return the minimum image box containing all image pixels."""
            max_pixel = (0, 0)
            min_pixel = min_map.size[:]
            for y in range(im.size[1]):
                for x in range(im.size[0]):
                    pixel = im.getpixel((x, y))
                    if pixel == 0 or pixel == (0, 0, 0):
                        max_pixel = (max(x, max_pixel[0]), max(y, max_pixel[1]))
                        min_pixel = (min(x, min_pixel[0]), min(y, min_pixel[1]))
            return im.crop((min_pixel[0], min_pixel[1], max_pixel[0] + 2, max_pixel[1] + 2,))

        with open(self.svg_file, "r") as svg:
            svg2 = BytesIO(svg.read().replace("stroke:#000000;", "").encode('ascii'))
            png = BytesIO()
            svg2png(file_obj=svg2, write_to=png, dpi=300)
        min_map = Image.open(png)
        new_image = Image.new("RGBA", min_map.size, "WHITE")
        new_image.paste(min_map, (0, 0), min_map)
        min_map = new_image.convert("1")
        min_map = get_min_image(min_map)
        min_output = get_min_image(output)

        min_map = min_map.resize(min_output.size)
        new_image = Image.new("1", self.size, "WHITE")
        new_image.paste(min_map, (self.size[0] - min_map.size[0], self.size[1] - min_map.size[1]))
        new_image.show()
        # im.show()
        # self.infill_map.show()
        self.infill_map = new_image
        return self.infill_map

    def add_infill(self, output, density=10):
        im = self.infill_map
        step = int(output.size[1] / (output.size[1] * (density / 100)))
        for y in range(0, output.size[1], step):
            start_x, end_x = None, None
            for x in range(output.size[0]):
                pixel = im.getpixel((x, y))
                if pixel == 0:
                    if start_x is None:
                        start_x = x
                    else:
                        end_x = x
                else:
                    if start_x is not None and end_x is not None:
                        path_string = "M {},{} L {},{}".format(start_x, y, end_x, y)
                        self.paths.append(Path(path_string=path_string, zoom=1))
                    start_x, end_x = None, None
            if start_x is not None and end_x is not None:
                path_string = "M {},{} L {},{}".format(start_x, y, end_x, y)
                self.paths.append(Path(path_string=path_string, zoom=1))

        return output

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
        # commands.append("M0.0,{}.0".format(self.bed_size[1]))

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
        self.pos_x = Decimal(0.0)
        self.pos_y = Decimal(0.0)
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
            args = [Decimal(f) for f in args.split(",")]
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
            else:
                print(command)

        command = ""
        for l in d:
            if l in "MmLlHhVvCcZzq":
                parse_command(command)
                command = l
            else:
                command += l
        parse_command(command)

    def goto_xy(self, x, y):
        self._all.append((ceil(x), ceil(y)))
        self.pos_x = x
        self.pos_y = y

    def cubic_bezier(self, x0, y0, x1, y1, x2, y2, x3, y3):
        delta = int(max(abs(x3 - x0), abs(y3 - y0)) * 2)
        for t in range(delta):
            t = Decimal(t) / Decimal(delta)
            x = (
                (Decimal(1) - t) ** Decimal(3) * x0
                + Decimal(3) * (Decimal(1) - t) ** Decimal(2) * t * x1
                + Decimal(3) * (Decimal(1) - t) * t ** Decimal(2) * x2
                + t ** Decimal(3) * x3
            )
            y = (
                (Decimal(1) - t) ** Decimal(3) * y0
                + Decimal(3) * (Decimal(1) - t) ** Decimal(2) * t * y1
                + Decimal(3) * (Decimal(1) - t) * t ** Decimal(2) * y2
                + t ** Decimal(3) * y3
            )
            self._line(self.pos_x, self.pos_y, x, y)

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
            self._line(x0, y0, x1, y1)
        #     if abs(dx) >= abs(dy):
        #         if x1 > x0:
        #             for mx in range(int(x0), int(x1)):
        #                 my = y0 + dy * (mx - x0) / dx
        #                 self.goto_xy(mx, my)
        #         else:
        #             for mx in range(int(x0), int(x1), -1):
        #                 my = y0 + dy * (mx - x0) / dx
        #                 self.goto_xy(mx, my)
        #     else:
        #         if y1 > y0:
        #             for my in range(int(y0), int(y1)):
        #                 mx = x0 + dx * (my - y0) / dy
        #                 self.goto_xy(mx, my)
        #         else:
        #             for my in range(int(y0), int(y1), -1):
        #                 mx = x0 + dx * (my - y0) / dy
        #                 self.goto_xy(mx, my)

        #     self.goto_xy(x1, y1)
        self.commands.append("L{:.1f},{:.1f}".format(x1, y1))

    def _line(self, x0, y0, x1, y1):
        dx = x1 - x0
        dy = y1 - y0
        if abs(dx) >= abs(dy):
            if x1 > x0:
                for mx in range(int(x0), int(x1)):
                    my = y0 + dy * (mx - x0) / dx
                    self.goto_xy(mx, my)
            else:
                for mx in range(int(x0), int(x1), -1):
                    my = y0 + dy * (mx - x0) / dx
                    self.goto_xy(mx, my)
        else:
            if y1 > y0:
                for my in range(int(y0), int(y1)):
                    mx = x0 + dx * (my - y0) / dy
                    self.goto_xy(mx, my)
            else:
                for my in range(int(y0), int(y1), -1):
                    mx = x0 + dx * (my - y0) / dy
                    self.goto_xy(mx, my)

        self.goto_xy(x1, y1)

    def Z(self):
        self.L(*self.start)
        self.start = None
        self.commands.append("Z")

    def draw(self, output):
        color = (0, 0, 0)
        for x, y in self._all:
            # output.putpixel((int(x) + self.move_x, int(y) + self.move_y), color)
            output.putpixel((int(x), int(y)), color)
        # for command in self.commands:
        #     print("{}({});".format(command[0], command[1:]))

        return output

    # def infill(self, output):
    #     return output

    #     def draw_line(y, x0, x1):
    #         for x in range(x0, x1 + 1):
    #             output.putpixel((x, y), (0, 0, 0))
    #         return output

    #     if not self.fill:
    #         return output

    #     scanlines = defaultdict(list)
    #     for x, y in self._all:
    #         scanlines[int(y)].append(int(x))
    #     scanlines = [[y, sorted(list(set(xs)))] for y, xs in sorted(scanlines.items())]
    #     for y, xs in scanlines:
    #         if len(xs) == 1:
    #             continue
    #         x0, x1 = None, None
    #         ready = False
    #         print(xs)
    #         # [139, 156, 157, 158, 385, 521, 522, 523]
    #         # 139 158
    #         for x in xs:
    #             if x0 is None:
    #                 x0 = x
    #                 continue
    #             else:
    #                 if x == x0 + 1:
    #                     x0 = x
    #                 elif x1 is None:
    #                     x1 = x
    #                 elif x1 is not None and x == x1 + 1:
    #                     x1 = x
    #                 elif x1 is not None and not x == x1 + 1:
    #                     ready = True
    #             if ready:
    #                 print(x0, x1)
    #                 output = draw_line(y, x0, x1)
    #                 x0, x1 = x, None
    #                 ready = False
    #         if x0 is not None and x1 is not None:
    #             print(x0, x1)
    #             output = draw_line(y, x0, x1)

    #     return output

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
