# Define the sequence to be written
sequence = ["p 1", "w 2", "p 2", "p 3"]

# Open a text file to write the sequence
with open("tasks_sequence.txt", "w") as file:
    # Repeat the sequence 250 times
    for _ in range(250):
        # Write each task in the sequence to the file
        for task in sequence:
            file.write(task + "\n")

print("Sequence written 250 times to tasks_sequence.txt")

