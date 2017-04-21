C:/Python36/python -m venv env
call env/scripts/activate.bat
C:/Python36/python -m pip install --upgrade pip
pip -q install -r requirements.txt
C:/Python36/python docgen.py