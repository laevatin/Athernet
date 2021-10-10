import numpy as np

def create_input(filepath, datasize):
    np.random.seed()
    file = open(filepath, "w")
    data = np.random.randint(2, size=datasize).tolist()
    # print(data)
    for i in range(datasize):
        file.write(str(data[i]))
    file.close()

def file_diff(a, b):
    input_file, output_file = open(a, "r"), open(b, "r")
    input_str, output_str = input_file.read(), output_file.read()
    input_arr, output_arr = np.array(list(input_str), dtype=np.uint8),  \
                                np.array(list(output_str), dtype=np.uint8)

    correct_num = np.sum(input_arr == output_arr)
    print("All Data Correct Rate: ", correct_num / input_arr.size * 100, "%", sep='')
    input_file.close()
    output_file.close()

create_input("input.in", 100)
