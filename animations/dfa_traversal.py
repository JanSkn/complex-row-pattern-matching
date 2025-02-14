"""
Run in terminal:  manim -pqh animations/dfa_traversal.py DFAPathAnimation
"""

from manim import *

class DFAPathAnimation(Scene):
    def construct(self):
        q0 = Circle(radius=0.5).shift(LEFT*3)
        q0_label = Text("q0", font_size=24).next_to(q0, DOWN, buff=0.1)
        
        q1 = Circle(radius=0.5)
        q1_label = Text("q1", font_size=24).next_to(q1, DOWN, buff=0.1)
        
        q2 = Circle(radius=0.5).shift(RIGHT*3)
        q2_label = Text("q2", font_size=24).next_to(q2, DOWN, buff=0.1)

        q3 = Circle(radius=0.5).shift(UP*2)
        q3_label = Text("q3", font_size=24).next_to(q3, UP, buff=0.1)

        start_arrow = Arrow(LEFT*4, LEFT*3.5, buff=0, stroke_width=4, tip_length=0.2)

        final_state = Circle(radius=0.6).move_to(q2.get_center())

        arrow_0_1 = Arrow(q0.get_right(), q1.get_left(), buff=0.1, stroke_width=4, tip_length=0.2)
        arrow_1_2 = Arrow(q1.get_right(), q2.get_left(), buff=0.1, stroke_width=4, tip_length=0.2)
        arrow_3_1 = Arrow(q3.get_bottom(), q1.get_top(), buff=0.1, stroke_width=4, tip_length=0.2)  
        arrow_0_3 = Arrow(q0.get_top(), q3.get_left(), buff=0.1, stroke_width=4, tip_length=0.2) 
        self_arrow = Arrow(
            q2.get_bottom(),
            q2.get_bottom() + DOWN*0.5 + RIGHT*0.5,
            path_arc=-3,
            buff=0.1,
            stroke_width=4,
            tip_length=0.2
        )

        label_0_1 = Text("A1", font_size=24).next_to(arrow_0_1, UP, buff=0.1)
        label_1_2 = Text("A2", font_size=24).next_to(arrow_1_2, UP, buff=0.1)
        label_3_1 = Text("B2", font_size=24).next_to(arrow_3_1, RIGHT, buff=0.1)
        label_0_3 = Text("B1", font_size=24).move_to(arrow_0_3.get_center() + UP * 0.4)  
        label_2_2 = Text("", font_size=24).next_to(self_arrow, DOWN, buff=0.1)

        self.play(
            Create(q0), Create(q1), Create(q2), Create(q3),
            Write(q0_label), Write(q1_label), Write(q2_label), Write(q3_label),
            Create(start_arrow), Create(final_state)
        )
        self.play(
            Create(arrow_0_1), Create(arrow_1_2), Create(arrow_3_1), Create(arrow_0_3), Create(self_arrow),
            Write(label_0_1), Write(label_1_2), Write(label_3_1), Write(label_0_3), Write(label_2_2)
        )
        self.wait(1)

        path_color = "#FF0000"

        self.play(
            q0.animate.set_fill(path_color, opacity=0.3),
            q0.animate.set_stroke(path_color),
        )
        self.play(
            arrow_0_1.animate.set_color(path_color),
            label_0_1.animate.set_color(path_color)
        )
        self.play(
            q1.animate.set_fill(path_color, opacity=0.3),
            q1.animate.set_stroke(path_color),
        )

        self.play(
            arrow_1_2.animate.set_color(path_color),
            label_1_2.animate.set_color(path_color)
        )
        self.play(
            q2.animate.set_fill(path_color, opacity=0.3),
            q2.animate.set_stroke(path_color),
            final_state.animate.set_stroke(path_color)
        )

        self.wait(1)

        self.play(
            *[mob.animate.set_color(WHITE) for mob in [
                q0, q1, q2, q3, arrow_0_1, arrow_1_2, arrow_3_1, arrow_0_3,
                label_0_1, label_1_2, label_3_1, label_0_3, final_state
            ]],
            *[mob.animate.set_fill(BLACK, opacity=0) for mob in [q0, q1, q2, q3]]
        )

        backward_color = "#00FF00"

        self.play(
            q2.animate.set_fill(backward_color, opacity=0.3),
            q2.animate.set_stroke(backward_color),
            final_state.animate.set_stroke(backward_color)
        )

        self.play(
            arrow_1_2.animate.set_color(backward_color),
            label_1_2.animate.set_color(backward_color),
            q1.animate.set_fill(backward_color, opacity=0.3),
            q1.animate.set_stroke(backward_color)
        )

        self.play(
            arrow_3_1.animate.set_color(backward_color),
            label_3_1.animate.set_color(backward_color),
            q3.animate.set_fill(backward_color, opacity=0.3),
            q3.animate.set_stroke(backward_color)
        )

        self.play(
            arrow_0_3.animate.set_color(backward_color),
            label_0_3.animate.set_color(backward_color),
            q0.animate.set_fill(backward_color, opacity=0.3),
            q0.animate.set_stroke(backward_color)
        )

        self.play(
            arrow_0_1.animate.set_color(backward_color),
            label_0_1.animate.set_color(backward_color)
        )

        self.play(
            self_arrow.animate.set_color(backward_color),
            label_2_2.animate.set_color(backward_color)
        )

        self.wait(2)

if __name__ == "__main__":
    scene = DFAPathAnimation()
    scene.render()