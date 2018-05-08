import random
import string

#Returns a random integer and prints to the screen
def randomNumber():
    foo = random.randint(1, 42)
    print(foo)
    return foo

#Returns a string containing exactly 10 random characters and a newline as the 11th character
def randomString():
    myString = ""
    for x in range(10):
        myString += random.choice(string.ascii_lowercase)
    print(myString)
    myString += '\n'
    return myString

#Create three file in the working directory and fills them with a random string
def openFiles():
    for x in range(3):
        f = open("newFile"+str(x), "w+")
        f.write(randomString())
        f.close()

def main():
    #Open three files and write into them a random string
    openFiles()

    #Print three random numbers and their product
    print(randomNumber()*randomNumber())

    #Note for TA
    print("\nFile names of created files start with newFile")

main()
