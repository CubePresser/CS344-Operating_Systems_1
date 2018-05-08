import random, string
for x in range(3):
    f = open("newFile"+str(x), "w+")
    myString= ''.join(random.choice(string.ascii_lowercase) for _ in range(10)) + '\n'
    print(myString),
    f.write(myString)
    f.close()
x , y = random.randint(1,42), random.randint(1,42)
print(str(x) + '\n' + str(y) + '\n' + str(x*y))
