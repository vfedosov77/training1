import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy


colors_bar1 = None
colors_bar2 = None
figure = None
plot1 = None
plot2 = None
pcolor1 = None
pcolor2 = None

def init_plot(x, y, z):
    global figure, plot1, plot2, colors_bar1, colors_bar2, pcolor1, pcolor2

    if figure is None:
        figure, (plot1, plot2) = plt.subplots(1, 2)
        plot1.set_title("policy")
        plot1.set_xlabel("X")
        plot1.set_ylabel("Y")
        plot2.set_title("values")
        plot2.set_xlabel("X")
        plot2.set_ylabel("Y")
        pcolor1 = plot1.pcolor(x, y, z)
        pcolor2 = plot2.pcolor(x, y, z)
        colors_bar1 = figure.colorbar(pcolor1, ax=plot1)
        colors_bar2 = figure.colorbar(pcolor2, ax=plot2)


def show_policy_2d(policy, action_id, range_x = (-1.0, 1.0), range_y = (-1.0, 1.0), steps_count = 20):
    global plot2, figure, colors_bar1, pcolor1

    step_x = (range_x[1] - range_x[0]) / steps_count
    step_y = (range_y[1] - range_y[0]) / steps_count

    x_array = [range_x[0] + step_x * id for id in range(steps_count + 1)]
    y_array = [range_y[0] + step_y * id for id in range(steps_count + 1)]

    colors_matrix = [[policy(range_x[0] + step_x * x_id, range_y[0] + step_y * y_id)[action_id].item()
                      for x_id in range(steps_count + 1)] for y_id in range(steps_count + 1)]

    init_plot(x_array, y_array, colors_matrix)

    plt.ion()
    colors_matrix = numpy.array(colors_matrix).ravel()
    pcolor1.set_array(colors_matrix)
    pcolor1.set_clim(colors_matrix.min(), colors_matrix.max())
    colors_bar1.update_normal(pcolor1)
    plt.autoscale()
    plt.draw()
    plt.pause(0.01)
    plt.ioff()


def show_values_2d(values, range_x = (-1.0, 1.0), range_y = (-1.0, 1.0), steps_count = 20):
    global colors_bar2, plot2, figure, pcolor2

    step_x = (range_x[1] - range_x[0]) / steps_count
    step_y = (range_y[1] - range_y[0]) / steps_count

    x_array = [range_x[0] + step_x * id for id in range(steps_count + 1)]
    y_array = [range_y[0] + step_y * id for id in range(steps_count + 1)]

    colors_matrix = [[values(range_x[0] + step_x * x_id, range_y[0] + step_y * y_id).item()
                      for x_id in range(steps_count + 1)] for y_id in range(steps_count + 1)]

    init_plot(x_array, y_array, colors_matrix)

    plt.ion()
    colors_matrix = numpy.array(colors_matrix).ravel()
    pcolor2.set_array(colors_matrix)
    pcolor2.set_clim(colors_matrix.min(), colors_matrix.max())
    colors_bar2.update_normal(pcolor2)
    plt.draw()
    plt.pause(0.01)
    plt.ioff()