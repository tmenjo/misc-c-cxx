#include <sys/queue.h>

#include <check.h>

struct int_elem {
	int elem_;
	STAILQ_ENTRY(int_elem) next_;
};
STAILQ_HEAD(int_head, int_elem);

START_TEST(test_fifo)
{
	struct int_head head;
	STAILQ_INIT(&head);
	ck_assert(STAILQ_EMPTY(&head));
	ck_assert_ptr_eq(NULL, STAILQ_FIRST(&head));

	struct int_elem a;
	a.elem_ = 'A';
	STAILQ_INSERT_TAIL(&head, &a, next_);
	ck_assert(!STAILQ_EMPTY(&head));

	struct int_elem b, c;
	b.elem_ = 'B';
	c.elem_ = 'C';
	STAILQ_INSERT_TAIL(&head, &b, next_);
	STAILQ_INSERT_TAIL(&head, &c, next_);

	const struct int_elem *i = STAILQ_FIRST(&head);
	ck_assert_int_eq('A', i->elem_);
	i = STAILQ_NEXT(i, next_);
	ck_assert_int_eq('B', i->elem_);
	i = STAILQ_NEXT(i, next_);
	ck_assert_int_eq('C', i->elem_);
	i = STAILQ_NEXT(i, next_);
	ck_assert_ptr_eq(NULL, i);

	ck_assert_int_eq('A', STAILQ_FIRST(&head)->elem_);
	STAILQ_REMOVE_HEAD(&head, next_);
	ck_assert_int_eq('B', STAILQ_FIRST(&head)->elem_);
	STAILQ_REMOVE_HEAD(&head, next_);
	ck_assert_int_eq('C', STAILQ_FIRST(&head)->elem_);
	STAILQ_REMOVE_HEAD(&head, next_);
	ck_assert(STAILQ_EMPTY(&head));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_fifo);

	Suite *const suite = suite_create("stailq");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
