import os
import glob
import dominate
from dominate.tags import *
from collections import defaultdict

import utils


def create_benchmark_sites(img_dir, benchmark_pngs):
    for bp in benchmark_pngs.items():
        doc = dominate.document(title=bp[0])

        with doc.head:
            link(rel="stylesheet", href="style.css")

        with doc.body:
            with div(cls="sidenav"):
                #div(img(src="viz/html/des_logo.png"), cls="logo-image")
                a("Home", href="index.html", cls="big")
                for b in benchmark_pngs.keys():
                    a(b, href=b + ".html")

            h1(bp[0])
            h4("Benchmark configs:")

            # TODO: put configs into table
            for config in utils.get_config_of_benchmark(bp[0]).items():
                p(config[0] + ": " + str(config[1]))

            for png in bp[1]:
                div(img(src=img_dir + png))

        with open("viz/html/" + bp[0] + ".html", "w") as file:
            file.write(doc.render())


def create_index_site(benchmark_names):
    doc = dominate.document(title="PerMA-Bench Results")

    with doc.head:
        link(rel="stylesheet", href="style.css")

    with doc.body:
        with div(cls="sidenav"):
            #div(img(src="viz/html/des_logo.png"), cls="logo-image")
            a("Home", href="index.html", cls="big")
            for b in benchmark_names:
                a(b, href=b + ".html")

        h1("PerMA-Bench Results")
        p("Put description text here.")

    with open("viz/html/index.html", "w") as file:
        file.write(doc.render())


def init(img_dir):
    # TODO: delete old png and html files?
    benchmark_pngs = defaultdict(list)
    for path in glob.glob(img_dir + "*.png"):
        png_name = os.path.basename(path)
        benchmark_name = png_name.split("_")[0]
        benchmark_pngs[benchmark_name].append(png_name)

    create_index_site(benchmark_pngs.keys())
    create_benchmark_sites(img_dir, benchmark_pngs)
