import logging
from openai import OpenAI
import os
import multiprocessing
from ChampSim_CRC2.simulate_champsim import simulate_main
import shutil
import datetime

def setup_logger():
    os.makedirs('logs', exist_ok=True)
    logger = logging.getLogger("cpp_gen_logger")
    logger.setLevel(logging.DEBUG)

    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    # File Handler
    file_handler = logging.FileHandler('logs/app.log')
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(formatter)

    # Console Handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_handler.setFormatter(formatter)

    logger.addHandler(file_handler)
    logger.addHandler(console_handler) #Comment if you don't want logs to be printed in the console 

    return logger

def validate_api_key(api_key):
    try:
        client = OpenAI(api_key=api_key)
        client.models.list()
        return True
    except Exception:
        return False

def get_file_content(filename):
    try:
        with open(filename, "r", encoding="utf-8") as file:
            content = file.read()
        return content
    except Exception:
        return False


def record_score(trace, type, score, filepath):
    """
    Records the score of the generated code.
    """
    if type == "Error":
        score = 0
    with open(filepath, "a") as f:
        f.write(f"{trace},{type},{score}\n")

def _worker(func, args, kwargs, queue):
    """
    Runs func(*args, **kwargs) and puts the result into queue.
    """
    try:
        result = func(*args, **kwargs)
        queue.put(("OK", result))
    except Exception as e:
        queue.put(("EXC", e))

def invoke_simulator(timeout_sec=300, *args, **kwargs):
    q = multiprocessing.Queue()

    p = multiprocessing.Process(target=_worker, args=(simulate_main, args, kwargs, q))
    p.start()
    p.join(timeout_sec)
    if p.is_alive():
        p.terminate()
        p.join()
        return False, '', "Timeout", ''
    
    status, result = q.get()
    if status == "OK":
        if len(result) == 0:
            return False, '', '', ''
        trace = result[0]["trace"]
        type = result[0]["type"]
        value = result[0]["value"]
        return True, trace, type, value
    else:
        return False, '', '', ''

def save_to_database(filename, score, input_dir, output_dir):
    timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")

    base, ext = os.path.splitext(filename)
    parts = base.split('_')
    parts[-1] = timestamp
    new_filename = '_'.join(parts) + f"_{score}" + ext

    shutil.copy2(f"{output_dir}/{filename}", f"{input_dir}/{new_filename}")