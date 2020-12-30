import os
import glob
import dominate
from dominate.tags import *
from collections import defaultdict

import raw_json_utils as raw_utils


def create_benchmark_sites(img_dir, benchmark_pngs):
    for bp in benchmark_pngs.items():
        doc = dominate.document(title="PerMA-Bench Results")

        with doc.head:
            link(rel="stylesheet", href="style.css")

        with doc.body:
            # add fixed sidebar
            with div(cls="sidenav"):
                # TODO: insert logo
                a("Home", href="index.html", cls="big")
                for b in benchmark_pngs.keys():
                    a(b, href=b + ".html")

            # place benchmark name as heading
            h1(bp[0])

            # check if first png name of current bm ends with "raw"
            is_raw_json = (bp[1][0][-7:-4] == "raw")

            # add table with configs (for raw jsons)
            if is_raw_json:
                h4("Benchmark Configs:")

                with table().add(tbody()):
                    for config in raw_utils.get_raw_configs(bp[0]).items():
                        l = tr()
                        l.add(td(config[0]))
                        l.add(td(config[1]))

            # place pngs
            h4("Benchmark Results:")
            last_first_arg = ""
            last_second_arg = ""

            for png in bp[1]:
                if not is_raw_json:
                    first_arg = png[:-4].split("-")[1]
                    second_arg = png[:-4].split("-")[2]

                    if first_arg != last_first_arg or second_arg != last_second_arg:
                        h5(f"({first_arg}, {second_arg})")

                    last_first_arg = first_arg
                    last_second_arg = second_arg

                div(img(src=img_dir + png))

        with open("viz/html/" + bp[0] + ".html", "w") as file:
            file.write(doc.render())


def create_index_site(benchmark_names):
    doc = dominate.document(title="PerMA-Bench Results")

    with doc.head:
        link(rel="stylesheet", href="style.css")

    with doc.body:
        # add fixed sidebar
        with div(cls="sidenav"):
            # TODO: insert logo
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
        benchmark_name = png_name.split("-")[0]
        benchmark_pngs[benchmark_name].append(png_name)

    create_index_site(benchmark_pngs.keys())
    create_benchmark_sites(img_dir, benchmark_pngs)
