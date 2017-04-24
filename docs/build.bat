python -m venv env
call env/scripts/activate.bat
python -m pip install --upgrade pip
pip -q install -r requirements.txt
python docgen.py