def on_step_callback(summary, details, kind=NORMAL_TEXT):
    global answer_found
    if summary.startswith("Found the answer:"):
        answer_found = True

    app.add_log_entry(summary, details, kind)


dispatcher.add_events_observer(on_step_callback)

def test():
    global answer_found

    questions = ["Where is the main view of the application defined?",
        "I have a bug: the camera focus does not work.",
        "Is there any code related to data bases?",
        "Where application stores data?",
        "In which folder application stores data?",
        "How the native code is integrated with Android?",
        "Where matrix multiplication is processed?",
        "How images keypoints are compared?",
        "What Series class is responsible for?",
        "What Figure class is responsible for?",
        "Where keypoints are drawn on a picture in the project?",
        "How the camera focus is processed?"]

    correct_count = 0
    results = defaultdict(float)

    for question in questions:
        answer_found = False
        app.add_role_message("Test",question)
        result = base.get_answer(question, [])

        if answer_found:
            correct_count += 1
            results[question] += 0.5

        answer_found = False
        app.add_role_message("Test", question)
        result = base.get_answer(question, [])

        if answer_found:
            correct_count += 1
            results[question] += 0.5

    print("Correct count = " + str(correct_count))

    for question in questions:
        val = results[question]
        print(f"{question}: {val}")