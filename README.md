# ePaper54L
## A hardware, firmware, and software demonstration of Nordic Semiconductor's nRF54L15 for use in large, low-power, bidirectional networks

## Get started:

```bash
$ git clone https://github.com/hlord2000/ePaper54L.git
$ cd ePaper54L
$ kicad-nightly ./hardware/nRF54L_ePaper.kicad_pro
```

OR

```bash
$ git clone https://github.com/hlord2000/ePaper54L.git
$ cd ePaper54L
$ python3 -m venv venv
$ source venv/bin/activate
$ pip install west
$ west init -l app
$ west update
$ pip install -r nrf/scripts/requirements.txt
$ pip install -r zephyr/scripts/requirements.txt
$ source zephyr/zephyr-env.sh
$ west zephyr-export
$ cd app
$ west build -b <your_board>
```
