#! /usr/bin/env python
import os
from decimal import Decimal
from io import BytesIO
from math import ceil, cos, sin, sqrt
from sys import argv
from time import sleep

from PIL import Image
from xml.dom import minidom
import serial
from tqdm import tqdm
from cairosvg import svg2png

PIXELS_PER_MM = Decimal(40)
MAX_BED_PIXELS = (10840, 7320)
MAX_BED = (MAX_BED_PIXELS[0] / PIXELS_PER_MM, MAX_BED_PIXELS[1] / PIXELS_PER_MM)


class SVG:
    # def __init__(self, svg_file, infill_file=None, bed_size=(Decimal(230), Decimal(181))):
    def __init__(self, svg_file, infill_file=None, bed_size=(Decimal(200), Decimal(150))):
        self.bed_size = (bed_size[0] * PIXELS_PER_MM, bed_size[1] * PIXELS_PER_MM)
        assert self.bed_size[0] <= MAX_BED_PIXELS[0], (
            "Bed_size cannot be bigger than %smm x %smm" % MAX_BED
        )
        assert self.bed_size[1] <= MAX_BED_PIXELS[1], (
            "Bed_size cannot be bigger than %smm x %smm" % MAX_BED
        )
        self.svg_file = svg_file
        self.infill_file = infill_file or svg_file
        doc = minidom.parse(svg_file)
        path_strings = [path.getAttribute('d') for path in doc.getElementsByTagName('path')]
        self.paths = []
        zoom = Decimal(1.0)
        for path_string in tqdm(path_strings, desc="Scalling"):
            p = Path(path_string, zoom=zoom)
            self.paths.append(p)
        self.size = self.get_size()
        zoom = min(self.bed_size[0] / self.size[0], self.bed_size[1] / self.size[1])
        self.paths = []
        for path_string in tqdm(path_strings, desc="Base paths"):
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
        for p in tqdm(self.paths, "Cutting"):
            output = p.draw(output)
        base_infill_map = self.make_infill_map(output, self.svg_file)
        shade_infill_map = self.make_infill_map(output, self.infill_file)

        output = Image.new("RGB", size, color=(255, 255, 255))
        # output = self.add_infill(base_infill_map, output, density=10)
        # output = self.add_vertical_infill(base_infill_map, output, step_mm=1.2)
        output = self.add_nautilus_infill(base_infill_map, output, step_mm=1.0)
        # output = self.add_diagonal_infill(shade_infill_map, output, step_mm=1.2, rotate=True)
        for p in tqdm(self.paths, desc="Drawing"):
            output = p.draw(output)
        output.show()

    def make_infill_map(self, output, infill_file):
        def get_min_image(im):
            """Return the minimum image box containing all image pixels."""
            max_pixel = (0, 0)
            min_pixel = min_map.size[:]
            for y in tqdm(range(im.size[1]), desc="Mapping infill"):
                for x in range(im.size[0]):
                    pixel = im.getpixel((x, y))
                    if pixel == 0 or pixel == (0, 0, 0):
                        min_pixel = (min(x, min_pixel[0]), min(y, min_pixel[1]))
                        max_pixel = (max(x, max_pixel[0]), y)

            return im.crop((min_pixel[0], min_pixel[1], max_pixel[0] + 2, max_pixel[1] + 2,))

        infill_file = infill_file or self.infill_file
        with open(infill_file, "r") as svg:
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
        return new_image

    def add_infill(self, infill_map, output, step_mm=1.0):
        im = infill_map
        step = Decimal(step_mm) * PIXELS_PER_MM
        for y in tqdm(range(0, output.size[1], int(step)), desc="Horizontal Infill"):
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

    def add_vertical_infill(self, infill_map, output, step_mm=1.0):
        im = infill_map
        step = Decimal(step_mm) * PIXELS_PER_MM
        for x in tqdm(range(0, output.size[0], int(step)), desc="Vertical Infill"):
            start_y, end_y = None, None
            for y in range(output.size[1]):
                pixel = im.getpixel((x, y))
                if pixel == 0:
                    if start_y is None:
                        start_y = y
                    else:
                        end_y = y
                else:
                    if start_y is not None and end_y is not None:
                        path_string = "M {},{} L {},{}".format(x, start_y, x, end_y)
                        self.paths.append(Path(path_string=path_string, zoom=1))
                    start_y, end_y = None, None
            if start_y is not None and end_y is not None:
                path_string = "M {},{} L {},{}".format(x, start_y, x, end_y)
                self.paths.append(Path(path_string=path_string, zoom=1))

        return output

    def add_diagonal_infill(self, infill_map, output, step_mm=1.0, rotate=False):
        im = infill_map
        step = Decimal(step_mm) * PIXELS_PER_MM
        y_range = range(0, output.size[1] + output.size[0], int(step))
        for y in tqdm(y_range, desc="Diagonal infill"):
            start_x_y, end_x_y = None, None
            x_range = range(min(y, output.size[0]))
            if rotate:
                x_range = range(output.size[0] - 1, 0, -1)
            for index, x in enumerate(x_range):
                _y = y - index
                if _y >= output.size[1] or _y < 0:
                    continue
                pixel = im.getpixel((x, _y))
                if pixel == 0:
                    if start_x_y is None:
                        start_x_y = (x, _y)
                    else:
                        end_x_y = (x, _y)
                else:
                    if start_x_y is not None and end_x_y is not None:
                        path_string = "M {},{} L {},{}".format(
                            start_x_y[0], start_x_y[1], end_x_y[0], end_x_y[1]
                        )
                        self.paths.append(Path(path_string=path_string, zoom=1))
                    start_x_y, end_x_y = None, None
            if start_x_y is not None and end_x_y is not None:
                path_string = "M {},{} L {},{}".format(
                    start_x_y[0], start_x_y[1], end_x_y[0], end_x_y[1]
                )
                self.paths.append(Path(path_string=path_string, zoom=1))

        return output

    def add_nautilus_infill(self, infill_map, output, step_mm=1.0):
        def pix_distance(a, b):
            catx = abs(a[0] - b[0])
            caty = abs(a[1] - b[1])
            return int(sqrt(catx * catx + caty * caty))

        im = infill_map
        step = Decimal(step_mm) * PIXELS_PER_MM
        min_movement = 1 * PIXELS_PER_MM
        self.paths = []

        center = (int(im.size[0] / 2), int(im.size[1] / 2))
        max_radius = Decimal(sqrt(im.size[0] * im.size[0] + im.size[1] * im.size[1]))
        grad_to_rad = 0.01745329252
        angle = 0.0
        radius = min_movement
        start_point = None
        end_point = None
        path_string = ""
        with tqdm(total=int(max_radius), desc="Nautilus infill") as pbar:
            while radius < max_radius:
                pbar.update(int(step))
                for angle in range(36000):
                    radius += step / 36000
                    angle = Decimal((angle / 100.0) * grad_to_rad)
                    x = int(center[0] + (radius * Decimal(cos(angle))))
                    y = int(center[1] + (radius * Decimal(sin(angle))))
                    if x >= im.size[0] or y >= im.size[1] or x < 0 or y < 0:
                        start_point = None
                        end_point = None
                        continue
                    pixel = im.getpixel((x, y))
                    if pixel == 0:
                        if start_point is None:
                            start_point = (x, y)
                            path_string += "M {},{}".format(x, y)
                        else:
                            end_point = (x, y)
                            if pix_distance(start_point, end_point) >= min_movement:
                                path_string += " L {},{}".format(end_point[0], end_point[1])
                                start_point = (x, y)
                                end_point = None

                    else:
                        if path_string:
                            self.paths.append(Path(path_string=path_string, zoom=1))
                        start_point = None
                        end_point = None
                        path_string = ""

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

        MAX_BUFFER_SIZE = 300
        MAX_BUFFER_SIZE = 1000
        messages = commands[0:2]

        payload = "{};\r\n".format(";".join(messages))
        ser.write(bytes(payload.encode()))
        while not ser.readline() == b'ack\r\n':
            pass

        messages = []
        for command in tqdm(commands[2:], desc="Printing"):
            if len("{};\r\n".format(";".join(messages + [command]))) >= MAX_BUFFER_SIZE:
                payload = "{};\r\n".format(";".join(messages))
                messages = [command]
                ser.write(bytes(payload.encode()))
                while not ser.readline() == b'ack\r\n':
                    pass
            else:
                messages.append(command)
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

        return output


if __name__ == "__main__":
    file_path = argv[1]
    infill = None
    if len(argv) == 3:
        infill = argv[2]
    s = SVG(svg_file=file_path, infill_file=infill)
    s.draw()
    s.plot()
