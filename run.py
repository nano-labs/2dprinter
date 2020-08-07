import io
from time import sleep
from random import choice, shuffle
import serial
from PIL import Image, ImageEnhance, ImageOps

MAX_X = 1400  # mm


def split_layers(im, layer_count=5):
    light, dark = 0, 255
    for x in range(im.size[0]):
        for y in range(im.size[1]):
            pix = im.getpixel((x, y))
            light = max(pix, light)
            dark = min(pix, dark)
    layer_depth = (light - dark) / layer_count
    layers = [
        ((dark + (l * layer_depth)), (dark + ((l + 1) * layer_depth))) for l in range(layer_count)
    ]
    results = []
    # layer_image = Image.new("1", im.size, color=0)
    final_image = Image.new("L", im.size, color=255)
    for depth, layer in enumerate(layers[:], 1):
        layer_image = Image.new("L", im.size, color=255)
        result = [[0 for _ in range(im.size[0])] for __ in range(im.size[1])]
        for x in range(im.size[0]):
            for y in range(im.size[1]):
                pix = im.getpixel((x, y))
                if pix > layer[0] and pix < layer[1]:
                    layer_image.putpixel((x, y), int(255 / (layer_count)) * (depth - 1))
                    final_image.putpixel((x, y), int(255 / (layer_count)) * (depth - 1))
                    result[y][x] = 1
        results.append(result)
        layer_image.show()
    final_image.show()
    return results

    # for r in results:
    #     print(r)
    # im.show()


class MazeEffect:
    def __init__(self, pixmap, pix_size=1.3, output=None, output_color=0, no_print=False):
        self.pixmap = pixmap[:]
        self.size = (len(pixmap[0]), len(pixmap))
        self.pix_size = pix_size
        if (self.pix_size * self.size[0]) > MAX_X:
            raise Exception(
                f"Size exceeds table size. For pix size "
                f"{self.pix_size} you max image X is {MAX_X / self.pix_size} pixels"
            )
        self.output = output or Image.new("L", (self.size[0] * 5, self.size[1] * 5), color=255)
        self.output_color = output_color
        self.cursor = (0, 0)
        self.pen = "up"
        self.ser = None
        if not no_print:
            self.ser = serial.Serial("/dev/tty.usbmodemFA131", 57600, timeout=1)
        self.command_queue = []
        self.serial_started = False
        self.set_pix_size()

    def set_pix_size(self):
        self.send_command("s{}".format(self.pix_size), flush=True)

    def send_command(self, command, flush=False):
        if self.ser and not self.serial_started:
            r = self.ser.readline()
            while not r == b'started\r\n':
                r = self.ser.readline()
            self.serial_started = True
        elif not self.ser:
            return

        self.command_queue.append(command)
        if len(self.command_queue) >= 100 or flush:
            self.command_queue.append("t")
            payload = ";".join(self.command_queue)
            payload = "{};\r\n".format(payload)
            print(payload)
            self.ser.write(bytes(payload.encode()))
            sleep(3)
            r = self.ser.readline()
            while not r == b'ready\r\n':
                r = self.ser.readline()
            self.command_queue = []

    def find_pix(self):
        def test_pix(px, py):
            try:
                if self.pixmap[py][px] == 1:
                    return True
            except IndexError:
                pass
            return False

        for y, line in enumerate(self.pixmap):
            for x in line:
                if self.pixmap[y][x] == 1:
                    self.pixmap[y][x] = 2
                    return (x, y)
        max_radius = self.cursor[0]
        max_radius = max(max_radius, self.cursor[1])
        max_radius = max(max_radius, self.size[0] - self.cursor[0])
        max_radius = max(max_radius, self.size[1] - self.cursor[1])
        max_radius = max(self.size)
        for r in range(max_radius + 1):
            possibles = []
            for dr in range(r + 1):
                x = self.cursor[0] + r
                y = self.cursor[1] - dr
                possibles.append([x, y])
                y = self.cursor[1] + dr
                possibles.append([x, y])
                x = self.cursor[0] - r
                y = self.cursor[1] - dr
                possibles.append([x, y])
                y = self.cursor[1] + dr
                possibles.append([x, y])

                y = self.cursor[1] + r
                x = self.cursor[0] - dr
                possibles.append([x, y])
                x = self.cursor[0] + dr
                possibles.append([x, y])
                y = self.cursor[1] - r
                x = self.cursor[0] - dr
                possibles.append([x, y])
                x = self.cursor[0] + dr
                possibles.append([x, y])
            shuffle(possibles)
            for x, y in possibles:
                if test_pix(x, y):
                    self.pixmap[y][x] = 2
                    return (x, y)
        return None

    def get_adjacent(self, pos=None):
        pos = pos or self.cursor
        ads = []
        for h, v in [(0, -1), (1, 0), (0, 1), (-1, 0)]:
            try:
                test_y = pos[1] + v
                test_x = pos[0] + h
                if test_x < 0 or test_y < 0:
                    continue
                if self.pixmap[pos[1] + v][pos[0] + h] == 1:
                    ads.append((pos[0] + h, pos[1] + v))
            except IndexError:
                pass
        return ads

    def move_to(self, pos, terminate=False):
        self.cursor = pos
        print(self.cursor)
        self.send_command("g{},{}".format(*pos), flush=terminate)
        # if self.ser:
        #     self.ser.write(bytes("g{},{}\n".format(*pos).encode()))
        #     r = self.ser.readline()
        #     while not r == b'ready\r\n':
        #         r = self.ser.readline()

    def draw_to(self, pos):
        line_x = pos[0] - self.cursor[0]
        line_y = pos[1] - self.cursor[1]
        if line_x == 1:
            for inc in range(6):
                self.output.putpixel(
                    ((self.cursor[0] * 5) + inc, (self.cursor[1] * 5)), self.output_color
                )
        if line_x == -1:
            for inc in range(6):
                self.output.putpixel(
                    ((self.cursor[0] * 5) - inc, (self.cursor[1] * 5)), self.output_color
                )
        if line_y == 1:
            for inc in range(6):
                self.output.putpixel(
                    ((self.cursor[0] * 5), (self.cursor[1] * 5) + inc), self.output_color
                )
        if line_y == -1:
            for inc in range(6):
                self.output.putpixel(
                    ((self.cursor[0] * 5), (self.cursor[1] * 5) - inc), self.output_color
                )

        self.pen_down()
        self.pixmap[self.cursor[1]][self.cursor[0]] = 2
        self.move_to(pos)
        self.pixmap[pos[1]][pos[0]] = 2

    def pen_down(self):
        if self.pen == "up":
            print("pen down")
            self.send_command("d")
            # if self.ser:
            #     self.ser.write(bytes("d".encode()))
            #     r = self.ser.readline()
            #     while not r == b'ready\r\n':
            #         r = self.ser.readline()
            self.pen = "down"

    def pen_up(self):
        if self.pen == "down":
            print("pen up")
            self.send_command("u")
            # if self.ser:
            #     self.ser.write(bytes("u".encode()))
            #     r = self.ser.readline()
            #     while not r == b'ready\r\n':
            #         r = self.ser.readline()
            self.pen = "up"

    def test_page(self):
        def sequence():
            self.pen_down()
            # direita
            self.draw_to((self.cursor[0] + 1, self.cursor[1]))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            # direita
            self.draw_to((self.cursor[0] + 1, self.cursor[1]))
            # cima
            self.draw_to((self.cursor[0], self.cursor[1] - 1))
            # direita
            self.draw_to((self.cursor[0] + 1, self.cursor[1]))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            # esquerda
            self.draw_to((self.cursor[0] - 1, self.cursor[1]))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            # esquerda
            self.draw_to((self.cursor[0] - 1, self.cursor[1]))
            # cima
            self.draw_to((self.cursor[0], self.cursor[1] - 1))
            # esquerda
            self.draw_to((self.cursor[0] - 1, self.cursor[1]))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            # baixo
            self.draw_to((self.cursor[0], self.cursor[1] + 1))
            self.pen_up()

        self.move_to((0, 0))
        self.pen_down()

        for _ in range(10):
            sequence()

        self.move_to((4, 0))
        for _ in range(10):
            sequence()

        self.move_to((8, 0))
        for _ in range(10):
            sequence()

        self.move_to((12, 0))
        for _ in range(10):
            sequence()

        self.move_to((16, 0))
        for _ in range(10):
            sequence()
        self.move_to((0, 0), terminate=True)

    def process(self):
        self.move_to((0, 0))
        pix = self.find_pix()
        self.pen_down()
        self.pen_up()
        while pix:
            ads = self.get_adjacent(pix)
            if ads:
                self.move_to(pix)
            while ads:
                next_pix = choice(ads)
                self.draw_to(next_pix)
                ads = self.get_adjacent()
            self.pen_up()
            pix = self.find_pix()
        self.output.show()
        self.pen_up()
        self.move_to((0, 0), terminate=True)
        return self.output


class DotEffect:
    def __init__(self, layers):
        self.layers = layers
        # self.ser = serial.Serial("/dev/tty.usbmodemFA131", 57600, timeout=1)

    def point(self, x, y):
        print("p{},{}\n".format(x, y))
        # self.ser.write(bytes("p{},{}\n".format(x, y).encode()))
        # r = self.ser.readline()
        # while not r == b'ready\r\n':
        #     r = self.ser.readline()

    def process_layer(self, layer):
        for y, line in enumerate(layer):
            for x, value in enumerate(line):
                if value == 1:
                    self.point(x, y)

    def process(self):
        for index, layer in enumerate(self.layers):
            for r in range(len(self.layers) - index):
                self.process_layer(layer)


def run():
    # me = MazeEffect([[0 for x in range(25)] for y in range(45)], pix_size=1.5, no_print=False)
    # me.test_page()
    # me.output.show()
    # return

    image = Image.open("img.jpg")
    image = image.convert("L")
    # image = ImageOps.invert(image)

    # image = ImageEnhance.Contrast(image).enhance(1.3)
    # output = None
    # layers = split_layers(image, 2)
    # for index, pmap in enumerate(layers):
    #     output_color = int(index * (255 / len(layers)))
    #     if input() == "s":
    #         continue
    #     me = MazeEffect(
    #         pmap, pix_size=0.95, output=output, output_color=output_color, no_print=True
    #     )
    #     output = me.process()
    layers = split_layers(image, 3)
    de = DotEffect(layers)
    de.process()


run()
