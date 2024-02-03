# Define the sequence to be repeated
sequence = ["p 1\n", "w 2\n", "p 2\n", "p 3\n"]

# Calculate the number of repetitions needed to exceed 400,000 tasks
tasks_needed = 400000
sequence_length = 4  # Number of tasks in one repetition of the sequence
repetitions_needed = (tasks_needed // sequence_length) + 1  # Ensure exceeding 400,000 tasks

# Open a file to write the sequence
with open("tasks_huge.txt", "w") as file:
    for _ in range(repetitions_needed):
        file.writelines(sequence)

print(f"Generated file with more than {tasks_needed} tasks.")

