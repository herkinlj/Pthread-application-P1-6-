# Python script to write the specified sequence enough times to produce a total of 256 tasks

# Define the sequence
sequence = ["p 1", "w 2", "p 2", "p 3"]

# Since each iteration of the sequence contains 4 tasks, to get 256 tasks, we need 64 repetitions
repetitions = 64  # 256 total tasks / 4 tasks per sequence

# Open a file to write
with open("sequence_tasks_256.txt", "w") as file:
    for _ in range(repetitions):
        for task in sequence:
            file.write(task + "\n")

print(f"Written {repetitions * len(sequence)} tasks to sequence_tasks_256.txt")

