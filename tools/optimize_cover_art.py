"""Generate delivery artwork with Pillow; keep original imagegen artwork intact.

CSS maximum widths at 2x: cards 823px, detail 968px, hero 1360px.
Run with .tools/venv/bin/python tools/optimize_cover_art.py.
"""
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1]
slugs=('dot-swarm','stormkite','comet-links','prism-well')
for slug in slugs:
    folder=root/'games'/slug/'art'
    im=Image.open(folder/'cover-illustrated-v1.png').convert('RGB')
    for kind,width in [('card',828),('detail',972),('share',1200)]+([('hero',1360)] if slug=='dot-swarm' else []):
        out=folder/f'cover-{kind}-v1.{"jpg" if kind=="share" else "webp"}'
        resized=im.resize((width,round(im.height*width/im.width)),Image.Resampling.LANCZOS)
        resized.save(out,quality=85,**({'optimize':True,'progressive':True} if kind=='share' else {'method':6}))
        print(out.name,slug,resized.size,out.stat().st_size)
im=Image.open(root/'collection/web/chromatic-fun-share-v3.png').convert('RGB')
im.resize((1200,628),Image.Resampling.LANCZOS).save(root/'collection/web/chromatic-fun-share-v3.jpg',quality=85,optimize=True,progressive=True)
