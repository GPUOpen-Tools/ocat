import CommonMark as cm
from jinja2 import Template
import glob
import os
import shutil
from yaml import load

def PrepareDocument (document):
    lines = document.split ('\n')
    
    header = []
    lastHeaderLine = -1
    if lines [0] == '---':
        for i, line in enumerate (lines):
            if i == 0:
                continue
            if line == '---':
                lastHeaderLine = i
                break
            else:
                header.append (line)
            
    return load ('\n'.join (header)), '\n'.join (lines [lastHeaderLine+1:])

if __name__=='__main__':
    pageTemplate = Template (open ('template.html', 'r').read ())

    shutil.rmtree ('output', ignore_errors=True)
    os.mkdir ('output')

    for file in glob.glob ('*.md'):
        header, document = PrepareDocument (open (file, 'r').read ())
        if header is None:
            header = {}
        content = cm.commonmark (document)
        open ('output/' + os.path.splitext (file)[0] + '.html', 'w').write (pageTemplate.render (content = content, **header))

    shutil.copy ('style.css', 'output/style.css')
    shutil.copy ('../LICENSE.txt', 'output/LICENSE.txt')