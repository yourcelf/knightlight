
red_count = 3
blue_count = 1
green_count = 1
total_count = red_count + blue_count + green_count

for j in range(100):
    chunk = j / total_count
    red_level = chunk * red_count
    green_level = chunk * green_count
    blue_level = chunk * blue_count

    for i in range(100):
        if i < red_level:
            print "R",
        if i < green_level:
            print "G",
        if i < blue_level:
            print "B",
        print "."
        

